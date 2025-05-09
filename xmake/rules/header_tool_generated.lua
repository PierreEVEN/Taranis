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

    before_buildcmd_file(function(target, batchcmds, header_path, opt)
        import("core.project.config")

        local gen_hpp_path, gen_cpp_path, include_path, _ = compute_generated_source_paths(target, header_path)
        local gen_object_path = target:objectfile(gen_cpp_path)
        local depend_file = target:dependfile(gen_object_path)
        target:add("files", gen_cpp_path)

        local build_instruction_file = gen_cpp_path .. ".htt"
        local build_instrs = io.open(build_instruction_file, "wb")
        build_instrs:write(path.absolute(header_path) .. "," .. path.absolute(gen_cpp_path) .. "," .. path.absolute(gen_hpp_path) .. "," .. include_path .. "," .. path.absolute(depend_file) .. "," .. path.absolute(object_file) .. "\r\n")
        build_instrs:close()

        local header_tool_path = path.join(target:configdir(), target:plat(), target:arch(), config.mode(), is_host("windows") and "header_tool.exe" or "header_tool");
        batchcmds:show_progress(opt.progress, "${color.build.object}generate.reflection %s", gen_cpp_path)
        batchcmds:vrunv(path.absolute(header_tool_path), {path.absolute(build_instruction_file)})

        if not os.exists(gen_cpp_path) then
            local empty_obj_file = io.open(gen_cpp_path, "w")
            empty_obj_file:close()
        end
        
        batchcmds:set_depmtime(os.mtime(gen_cpp_path))
        batchcmds:set_depcache(target:dependfile(gen_cpp_path))
        batchcmds:add_depfiles(header_path)
    end)

    on_buildcmd_file(function(target, batchcmds, header_path, opt)
        local _, gen_cpp_path, _, _ = compute_generated_source_paths(target, header_path)
        local gen_object_path = target:objectfile(gen_cpp_path)

        batchcmds:show_progress(opt.progress, "${color.build.object}compile.reflection %s", header_path)
        batchcmds:compile(gen_cpp_path, gen_object_path, { sourcekind = "cxx" })

        table.insert(target:objectfiles(), gen_object_path)

        batchcmds:set_depmtime(os.mtime(gen_object_path))
        batchcmds:set_depcache(target:dependfile(gen_object_path))
        batchcmds:add_depfiles(header_path)
    end)
end)
