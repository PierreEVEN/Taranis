-- Generate and compile source files for the reflection system
rule("header.tool.generated", function(_)
    set_extensions(".hpp")

    -- Get the output paths for generated files for the given input header
    local function compute_generated_source_paths(target, source_header)
        -- Guess generated source file path
        local generated_path = path.relative(source_header, target:scriptdir())
        -- this is the include string the user should have added to it's class
        local include_path = generated_path
        local path_parts = path.split(generated_path)
        if (path_parts[1] == "public" or path_parts[1] == "private") then
            -- private/public directories are not required
            table.remove(path_parts, 1) -- remove public or private directory from path
            include_path = table.concat(path_parts, '/')
        end
        local basename = path.basename(source_header);
        local directory = path.directory(source_header);
        local generated_source = target:autogenfile(path.join(directory:gsub("public", "private", 1), basename .. ".gen.cpp"))
        local generated_header = target:autogenfile(path.join(directory:gsub("private", "public", 1), basename .. ".gen.hpp"))

        return generated_header, generated_source, include_path, generated_path
    end


    on_config(function (target)
        local path = target:autogenfile(path.join(path.relative(target:scriptdir(), os.projectdir()), "public"))
        os.mkdir(path)
        target:add("includedirs", path, { public = true })
    end)

    before_buildcmd_file(function(target, batchcmds, header_path, opt)
        import("core.project.config")

        local gen_hpp_path, gen_cpp_path, include_path, _ = compute_generated_source_paths(target, header_path)
        target:add("files", gen_cpp_path)

        local header_tool_path = path.join(target:configdir(), target:plat(), target:arch(), config.mode(), is_host("windows") and "header_tool.exe" or "header_tool");
        batchcmds:show_progress(opt.progress, "${color.build.object}generate.reflection %s", header_path)
        batchcmds:vrunv(path.absolute(header_tool_path), {path.absolute(header_path), path.absolute(gen_cpp_path), path.absolute(gen_hpp_path), include_path})

         -- Create an empty .gen.cpp file even if we doesn't need to generate reflection data for it
        if not os.exists(gen_cpp_path) then
            io.open(gen_cpp_path, "w"):close()
        end
        
        batchcmds:set_depmtime(os.mtime(gen_cpp_path))
        batchcmds:set_depcache(target:dependfile(gen_cpp_path))
        batchcmds:add_depfiles(header_path)
    end)

    on_buildcmd_file(function(target, batchcmds, header_path, opt)
        local _, gen_cpp_path, _, _ = compute_generated_source_paths(target, header_path)
        local gen_object_path = target:objectfile(gen_cpp_path)

        batchcmds:show_progress(opt.progress, "${color.build.object}compile.reflection %s", gen_cpp_path)
        batchcmds:compile(gen_cpp_path, gen_object_path, { sourcekind = "cxx" })

        local contains_object = false
        for _, v in ipairs(target:objectfiles()) do
            if v == gen_object_path then
                contains_object = true
                break
            end
        end
        if not contains_object then
            table.insert(target:objectfiles(), gen_object_path)
        end

        batchcmds:set_depmtime(os.mtime(gen_object_path))
        batchcmds:set_depcache(target:dependfile(gen_object_path))
        batchcmds:add_depfiles(header_path)
    end)
end)
