-- Generate and compile source files for the reflection system
rule("header.tool.generated", function(_)
    set_extensions(".hpp")

    -- Get the output paths for generated sources for the given input header
    local function compute_generated_source_paths(target, source_header)
        -- Guess generated source file path
        local generated_path = string.sub(os.projectdir() .. "/" .. source_header, string.len(target:scriptdir()) + 2)

        -- this is the include string the user should have added to it's class
        local include_path = generated_path
        if generated_path:match("^[^\\]+\\(.*)$") then
            include_path = generated_path:match("^[^\\]+\\(.*)$"):gsub("\\", "/")
        elseif generated_path:match("^[^/]+/(.*)$") then
            include_path = generated_path:match("^[^/]+/(.*)$"):gsub("/", "/")
        else
            raise("Error : no match for include path " .. include_path)
        end

        -- Generated source file path : replace .hpp extension with .gen.cpp
        local generated_source = target:autogendir() .. "/" .. string.sub(generated_path, 1, string.len(generated_path) - 3) .. "gen.cpp"
        -- generated classes are always private
        generated_source = generated_source:gsub("public", "private", 1)

        -- Generated header file path : replace .hpp extension with .gen.hpp
        local generated_header = target:autogendir() .. "/" .. string.sub(generated_path, 1, string.len(generated_path) - 3) .. "gen.hpp"
        -- generated headers are always public
        generated_header = generated_header:gsub("private", "public", 1)

        return generated_header, generated_source, include_path, generated_path
    end

    local function table_diff(t1, t2, compiler)
        local diff = {}
        local count2 = {}

        -- Build a frequency map of elements in t2
        for _, v in ipairs(t2) do
            count2[v] = (count2[v] or 0) + 1
        end

        -- Compare against t1, decrement count if found, otherwise it's unique
        for _, v in ipairs(t1) do
            if count2[v] and count2[v] > 0 then
                count2[v] = count2[v] - 1
            elseif v ~= compiler then
                table.insert(diff, v)
            end
        end

        return diff
    end

    before_buildcmd_files(function(target, batch_cmds, source_batch, opt)
        import("core.tool.compiler")
        import("core.project.config")

        -- build compile batch
        local args = {}
        local compinst = compiler.load("cxx", { target = target })

        local compflags = compinst:compflags({ target = target, configs = opt.configs })

        for _, header_path in ipairs(source_batch.sourcefiles) do
            local gen_hpp_path, gen_cpp_path, include_path, _ = compute_generated_source_paths(target, header_path)
            local gen_object_path = target:objectfile(gen_cpp_path)
            local depend_path = target:dependfile(gen_object_path)
            table.insert(args, "-f")
            table.insert(args, header_path)
            table.insert(args, include_path)
            table.insert(args, depend_path)
            table.insert(args, gen_hpp_path)
            table.insert(args, gen_cpp_path)
            table.insert(args, gen_object_path)

            if not os.exists(gen_object_path) then
                local empty_obj_file = io.open(gen_object_path, "w")
                empty_obj_file:close()
            end

            local contains = false
            for _, p in pairs(target:objectfiles()) do
                if p == gen_object_path then
                    contains = true
                    break
                end
            end
            if not contains then
                table.insert(target:objectfiles(), gen_object_path)
            end
            -- filter source specific arguments
            local file_args = table.join(compinst:compargv(gen_cpp_path, gen_object_path, { target = target, sourcefile = generated_source, configs = opt.configs }));

            -- add source specific arguments
            for _, v in pairs(table_diff(file_args, compflags, compinst:program())) do
                table.insert(args, "-o")
                table.insert(args, v)
            end
        end

        local bin_dir = target:configdir() .. "/" .. target:plat() .. "/" .. target:arch() .. "/" .. config.mode()

        table.insert(args, "-t")
        local header_tool_path = bin_dir .. "/header_tool"
        if is_plat("windows") then
            header_tool_path = header_tool_path .. ".exe"
        end
        table.insert(args, header_tool_path)

        table.insert(args, "-c")
        table.insert(args, compinst:program())
        for _, flag in pairs(compflags) do
            table.insert(args, flag)
        end

        -- run header scanner
        local header_scanner_path = bin_dir .. "/header_scanner"
        if is_plat("windows") then
            header_scanner_path = header_scanner_path .. ".exe"
        end
        batch_cmds:vexecv(header_scanner_path, args)
    end)


    --[[
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
        local depvalues, compflags, compinst = get_compiler_info(compiler, target, generated_source, opt)

        -- Load existing dep infos (or create if not exists)
        local objectfile = target:objectfile(generated_source)
        local dependfile = target:dependfile(objectfile)
        local lastmtime = os.isfile(objectfile) and os.mtime(objectfile) or os.isfile(dependfile) and os.mtime(dependfile) or 0
        local dependinfo = target:is_rebuilt() and {} or (depend.load(dependfile, {target = target}) or {})

        -- Test if source file was modified (otherwise skip it)
        if not depend.is_changed(dependinfo, {lastmtime = lastmtime, values = depvalues}) then
            -- ensure .obj is included in the project
            local contains = false
            for k, p in pairs(target:objectfiles()) do
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

        local header_tool_path = "$(buildir)/$(plat)/$(arch)/$(mode)/header_tool"
        if is_plat("windows") then header_tool_path = header_tool_path..".exe" end

        -- There are some cases where before_build is called during the generator phase, where the header_tool have still not been built.
        if not os.exists(header_tool_path) then
            wprint("header_tool is required but not built. Trying to build header_tool...")
            os.exec("xmake build header_tool")
        end

        -- Generate reflection sources using header tool
        -- print("$(buildir)/$(plat)/$(arch)/$(mode)/header_tool "..source_header.." "..generated_source.." "..generated_header.." "..include_path)
        batchcmds:show_progress(opt.progress, "${color.build.object}generate.reflection %s", source_header)
        os.exec(header_tool_path.." "..source_header.." "..generated_source.." "..generated_header.." "..include_path)
        --target:add("files", generated_source)
    end)

    on_buildcmd_file(function (target, batchcmds, source_header, opt)
        import("core.tool.compiler")
        import("core.project.depend")

        local generated_header, generated_source, include_path, generated_path = compute_generated_source_paths(target, source_header)
        --local depvalues, compflags, compinst = get_compiler_info(compiler, target, generated_source, opt)

        local compinst = compiler.load("cxx", {target = target})
        local compflags = compinst:compflags({target = target, sourcefile = generated_source, configs = opt.configs})
        local depvalues = {compinst:program(), compflags}

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
            })]]--[[

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
    end)]]
end)
