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

        return generated_header, generated_source, include_path
    end

    on_config(function(target)
        -- add include dir to generated headers
        local include_path = target:autogenfile(path.join(path.relative(target:scriptdir(), os.projectdir()), "public"))
        os.mkdir(include_path)
        target:add("includedirs", include_path, { public = true })

        -- Add generated cpp files (generate empty one if not exists)
        for _, file in pairs(os.files(target:scriptdir() .. "/**.hpp")) do
            local _, generated_source, _ = compute_generated_source_paths(target, path.relative(file, os.projectdir()))

            -- Create an empty .gen.cpp file even if we doesn't need to generate reflection data for it
            if not os.exists(generated_source) then
                io.open(generated_source, "w"):close()
            end
            target:add("files", generated_source, { always_added = true })
        end
    end)

    before_buildcmd_file(function(target, batchcmds, header_path, opt)
        import("core.project.config")

        local gen_hpp_path, gen_cpp_path, include_path = compute_generated_source_paths(target, header_path)
        local depend_file = target:dependfile(gen_cpp_path)

        local header_tool_path = path.join(target:configdir(), target:plat(), target:arch(), config.mode(), is_host("windows") and "header_tool.exe" or "header_tool");

        if not os.exists(header_tool_path) then
            wprint("Header is not compiled yet !")
            os.exec("xmake build header_tool")
        end

        batchcmds:show_progress(opt.progress, "${color.build.object}generate.reflection %s", header_path)
        batchcmds:vrunv(path.absolute(header_tool_path), { path.absolute(header_path), path.absolute(gen_cpp_path), path.absolute(gen_hpp_path), include_path })

        batchcmds:set_depmtime(os.mtime(gen_cpp_path))
        batchcmds:add_depfiles(header_path)
        batchcmds:set_depcache(depend_file)
    end)
end)
