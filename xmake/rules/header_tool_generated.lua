-- Generate and compile source files for the reflection system
rule("header.tool.generated", function(_)
    set_extensions(".hpp")

    -- Get the output paths for generated sources for the given input header
    local function compute_generated_source_paths(target, source_header)
        -- Guess generated source file path
        local generated_path = path.relative(source_header, target:scriptdir())

        -- this is the include string the user should have added to it's class
        local include_path = generated_path
        local path_parts = path.split(generated_path)
        if (path_parts[1] == "public" or path_parts[1] == "private") then -- private/public directories are not required
            table.remove(path_parts, 1) -- remove public or private directory from path
            include_path = table.concat(path_parts, '/')
        end
        print(include_path)

        local basename = path.basename(source_header);
        local directory = path.directory(source_header);
        local generated_source = target:autogenfile(path.join(directory:gsub("public", "private", 1), basename .. ".gen.cpp"))
        local generated_header = target:autogenfile(path.join(directory:gsub("private", "public", 1), basename.. ".gen.hpp"))

        return generated_header, generated_source, include_path, generated_path
    end

    --[[
    before_buildcmd_files(function(target, batch_cmds, source_batch, opt)
        import("core.project.config")

        local bin_dir = target:configdir() .. "/" .. target:plat() .. "/" .. target:arch() .. "/" .. config.mode()
        local header_tool_path = bin_dir .. "/header_tool"
         if is_plat("windows") then
             header_tool_path = header_tool_path .. ".exe"
         end

         if not os.exists(path.absolute(header_tool_path)) then
             wprint("header_tool is required but not built. Trying to build header_tool...")
             os.exec("xmake build header_tool")
         end

        local tmp_file_path = target:autogendir() .. "/header_tool_targets.htt"
        local tmp_file = io.open(tmp_file_path, "wb")

        -- build compile batch
        for _, header_path in ipairs(source_batch.sourcefiles) do
            local gen_hpp_path, gen_cpp_path, include_path, _ = compute_generated_source_paths(target, header_path)
            target:add("files", gen_cpp_path)

            if not os.exists(gen_cpp_path) then
                local empty_obj_file = io.open(gen_cpp_path, "w")
                empty_obj_file:close()
            end

            local objectfile = target:objectfile(gen_cpp_path)
            local dependfile = target:dependfile(objectfile)

            tmp_file:write(path.absolute(header_path) .. "," .. path.absolute(gen_cpp_path) .. "," .. path.absolute(gen_hpp_path) .. "," .. include_path .. "," .. dependfile .. "," .. objectfile .."\r\n")
        end
        tmp_file:close()

        --batch_cmds:show_progress(opt.progress, "${color.build.object}DONE.PRE %s", path.absolute(header_tool_path) .. " " .. path.absolute(tmp_file_path))
        batch_cmds:vexecv(path.absolute(header_tool_path) .. " " .. path.absolute(tmp_file_path))
        --batch_cmds:show_progress(opt.progress, "${color.build.object}DONE.reflection")
    end)

    on_buildcmd_file(function(target, batch_cmds, header_path, opt)
        import("core.tool.compiler")

        local compinst = compiler.load("cxx", {target = target})
        local compflags = compinst:compflags({target = target, sourcefile = gen_cpp_path, configs = opt.configs})
        local depvalues = {compinst:program(), compflags}

        local _, gen_cpp_path, _, _ = compute_generated_source_paths(target, header_path)
        local gen_object_path = target:objectfile(gen_cpp_path)
        batch_cmds:show_progress(opt.progress, "${color.build.object}compile.reflection %s", header_path)
        batch_cmds:compile(gen_cpp_path, gen_object_path, { sourcekind = "cxx" })

        local depend_file = target:dependfile(gen_object_path);
        batch_cmds:add_depvalues(depvalues)
        batch_cmds:add_depfiles(header_path)
        batch_cmds:set_depmtime(os.mtime(gen_object_path))
        batch_cmds:set_depcache(depend_file)

        if (os.exists(target:dependfile(gen_object_path))) then
            --print("OK "..target:dependfile(gen_object_path))
        else
            --print("BORDERL "..target:dependfile(gen_object_path))
        end
    end)--]]

    -- Get compiler details
    local function get_compiler_info(compiler, target, generated_source, opt)
        local compinst = compiler.load("cxx", {target = target})
        local compflags = compinst:compflags({target = target, sourcefile = generated_source, configs = opt.configs})
        local depvalues = {compinst:program(), compflags}
        return depvalues, compflags, compinst
    end

    before_buildcmd_file(function (target, batchcmds, source_header, opt)
        import("core.tool.compiler")
        import("core.project.depend")

        local generated_header, generated_source, include_path, generated_path = compute_generated_source_paths(target, source_header)
        local depvalues, _, _ = get_compiler_info(compiler, target, generated_source, opt)

        -- Load existing dep infos (or create if not exists)
        local objectfile = target:objectfile(generated_source)
        local dependfile = target:dependfile(objectfile)
        local lastmtime = os.isfile(objectfile) and os.mtime(objectfile) or os.isfile(dependfile) and os.mtime(dependfile) or 0
        local dependinfo = target:is_rebuilt() and {} or (depend.load(dependfile, {target = target}) or {})

        -- Test if source file was modified (otherwise skip it)
        if not depend.is_changed(dependinfo, {lastmtime = lastmtime, values = depvalues}) then
            -- ensure .obj is included in the project
            local contains = false
            for _, p in pairs(target:objectfiles()) do
                if p == objectfile then
                    contains = true
                    break
                end
            end
            if not contains then
                table.insert(target:objectfiles(), objectfile)
            end
            return
        end

        local header_tool_path = "$(builddir)/$(plat)/$(arch)/$(mode)/header_tool"
        if is_plat("windows") then header_tool_path = header_tool_path..".exe" end

        -- There are some cases where before_build is called during the generator phase, where the header_tool have still not been built.
        if not os.exists(header_tool_path) then
            wprint("header_tool is required but not built. Trying to build header_tool...")
            os.exec("xmake build header_tool")
        end

        local tmp_file_path = generated_source .. ".htt"
        local tmp_file = io.open(tmp_file_path, "wb")
        tmp_file:write(path.absolute(source_header) .. "," .. path.absolute(generated_source) .. "," .. path.absolute(generated_header) .. "," .. include_path .. "," .. dependfile .. "," .. objectfile .."\r\n")
        tmp_file:close()

        -- Generate reflection sources using header tool
        batchcmds:show_progress(opt.progress, "${color.build.object}generate.reflection %s", source_header)
        os.exec(header_tool_path.." "..tmp_file_path)
    end)

    on_buildcmd_file(function (target, batchcmds, source_header, opt)
        import("core.tool.compiler")
        import("core.project.depend")

        local _, generated_source, _, _ = compute_generated_source_paths(target, source_header)
        local depvalues, compflags, compinst = get_compiler_info(compiler, target, generated_source, opt)
        local objectfile = target:objectfile(generated_source)
        local dependfile = target:dependfile(objectfile)

        -- if header_tool generated a source file to build
        if (os.exists(generated_source)) then
            -- Load existing dep infos (or create if not exists)
            local dependinfo = target:is_rebuilt() and {} or (depend.load(dependfile, {target = target}) or {})

            -- Test if source file was modified (otherwise skip it)
            local lastmtime = os.isfile(objectfile) and os.mtime(objectfile) or 0
            if not depend.is_changed(dependinfo, {lastmtime = lastmtime, values = depvalues}) then
                return
            end

            -- Compile the generated source file
            batchcmds:show_progress(opt.progress, "${color.build.object}compiling.$(mode) %s", generated_source)
            assert(compinst:compile(generated_source, objectfile, {dependinfo = dependinfo, compflags = compflags}))
            --[[batchcmds:compile(generated_source, objectfile, {
                compiler = compinst,
                dependinfo = dependinfo,
                compflags = compflags
            })]]

            -- store build depvalues to detect depvalues changes
            dependinfo.values = depvalues
            depend.save(dependinfo, dependfile)
            table.insert(target:objectfiles(), objectfile)
        else -- We can ignore this file until it get changed
            -- save last update check time
            local dependinfo = {}
            dependinfo.files = {source_header}
            dependinfo.values = depvalues
            depend.save(dependinfo, dependfile)
        end
    end)
end)
