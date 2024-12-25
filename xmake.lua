add_rules("mode.debug", "mode.release")
add_rules("plugin.vsxmake.autoupdate")

set_project("TaranisEngine")
set_languages("clatest", "cxx20")
set_allowedarchs("windows|x64")
set_warnings("allextra")
set_allowedmodes("debug", "release")
set_defaultmode("release")
set_rundir(".")
set_runtimes(is_mode("debug") and "MTd" or "MT")
		
DEBUG = false;
BUILD_MONOLITHIC = false;

option("build-tests", { default = false })
option("build-monolithic", { default = true })

if is_mode("debug") then
    --set_optimize("fastest")
end

if is_mode("release") then
    set_symbols("hidden")
    set_optimize("fastest")
    set_strip("all")
end


add_defines("ENABLE_VALIDATION_LAYER")
add_defines("ENABLE_PROFILER")

add_requires("vulkan-loader", "glm", "imgui docking", "vulkan-memory-allocator", "concurrentqueue", "unordered_dense", "nativefiledialog-extended")
add_requires("glfw", {configs = {shared = true}})
add_requires("assimp", {configs = {shared = true, no_export = true}})
add_requires("freeimage", {configs = {rgb = true, shared = true}})
add_requires("slang master", {verify = false})

rule("generated_cpp", function (rule)
    set_extensions(".hpp")
    before_buildcmd_file(function (target, batchcmds, source_header, opt)
        import("core.tool.compiler")
        import("core.project.depend")

        -- Guess generated source file path
        local generated_path = string.sub(os.projectdir().."/"..source_header, string.len(target:scriptdir()) + 2)

        -- this is the include string the user should have added to it's class
        local include_path = generated_path:match("^[^\\]+\\(.*)$"):gsub("\\", "/")

        -- replace .hpp extension with .gen.cpp
        local generated_source = target:autogendir().."/"..string.sub(generated_path, 1, string.len(generated_path) - 3).."gen.cpp"
        -- generated classes are always private
        generated_source = generated_source:gsub("public", "private", 1)
        
        -- replace .hpp extension with .gen.hpp
        local generated_header = target:autogendir().."/"..string.sub(generated_path, 1, string.len(generated_path) - 3).."gen.hpp"
        -- generated headers are always public
        generated_header = generated_header:gsub("private", "public", 1)

        local compinst = compiler.load("cxx", {target = target})
        local compflags = compinst:compflags({target = target, sourcefile = generated_source, configs = opt.configs})

        local objectfile = target:objectfile(generated_source)
        local dependfile = target:dependfile(objectfile)
        -- Load existing dep infos (or create if not exists)
        local dependinfo = target:is_rebuilt() and {} or (depend.load(dependfile, {target = target}) or {})

        local depvalues = {compinst:program(), compflags}
        local lastmtime = os.isfile(objectfile) and os.mtime(objectfile) or os.isfile(dependfile) and os.mtime(dependfile) or 0

        -- Test if source file was modified (otherwise skip it)
        if not depend.is_changed(dependinfo, {lastmtime = lastmtime, values = depvalues}) then
            if (os.exists(generated_source)) then
                table.insert(target:objectfiles(), objectfile)
            end
            return
        end

        --print("$(buildir)/$(plat)/$(arch)/$(mode)/header_tool "..source_header.." "..generated_source.." "..generated_header.." "..include_path)

        -- Generate reflection header
        batchcmds:show_progress(opt.progress, "${color.build.object}generate.reflection %s", source_header)
        os.exec("$(buildir)/$(plat)/$(arch)/$(mode)/header_tool "..source_header.." "..generated_source.." "..generated_header.." "..include_path)
    end)

    
    on_buildcmd_file(function (target, batchcmds, source_header, opt)
        import("core.tool.compiler")
        import("core.project.depend")

        -- Guess generated source file path
        local generated_path = string.sub(os.projectdir().."/"..source_header, string.len(target:scriptdir()) + 2)

        -- this is the include string the user should have added to it's class
        local include_path = generated_path:match("^[^\\]+\\(.*)$"):gsub("\\", "/")

        -- replace .hpp extension with .gen.cpp
        local generated_source = target:autogendir().."/"..string.sub(generated_path, 1, string.len(generated_path) - 3).."gen.cpp"
        -- generated classes are always private
        generated_source = generated_source:gsub("public", "private", 1)
        
        local compinst = compiler.load("cxx", {target = target})
        local compflags = compinst:compflags({target = target, sourcefile = generated_source, configs = opt.configs})
        local depvalues = {compinst:program(), compflags}

        local objectfile = target:objectfile(generated_source)
        local dependfile = target:dependfile(objectfile)
        -- if exists
        if (os.exists(generated_source)) then

            -- replace .hpp extension with .gen.hpp
            local generated_header = target:autogendir().."/"..string.sub(generated_path, 1, string.len(generated_path) - 3).."gen.hpp"
            -- generated headers are always public
            generated_header = generated_header:gsub("private", "public", 1)

            -- Load existing dep infos (or create if not exists)
            local dependinfo = target:is_rebuilt() and {} or (depend.load(dependfile, {target = target}) or {})

            local lastmtime = os.isfile(objectfile) and os.mtime(objectfile) or 0

            -- Test if source file was modified (otherwise skip it)
            if not depend.is_changed(dependinfo, {lastmtime = lastmtime, values = depvalues}) then
                return
            end

            batchcmds:show_progress(opt.progress, "${color.build.object}compiling.$(mode) %s", generated_source)
            -- Compile and fill the dependency list into dependinfo
            assert(compinst:compile(generated_source, objectfile, {dependinfo = dependinfo, compflags = compflags}))
            
            -- store build depvalues to detect depvalues changes
            dependinfo.values = depvalues
            depend.save(dependinfo, dependfile)
            table.insert(target:objectfiles(), objectfile)
        else
            -- save last update check time
            local dependinfo = {}
            dependinfo.files = {source_header}
            dependinfo.values = depvalues
            depend.save(dependinfo, dependfile)    
        end
    end)
end)


function declare_module(module_name, opts)

    if (opts == nil) then
        print("Error : invalid options for module "..module_name)
        os.exit(-1)
    end
    
    local deps = opts.deps or {}
    local packages = opts.packages or {}
    local is_executable = opts.is_executable or false
    local enable_reflection = opts.enable_reflection or false
    local allow_shared_build = opts.allow_shared_build or false
    
    if DEBUG then
        print("### "..module_name.." ###")
    end
    target(module_name, function (target)
        add_defines("GLM_FORCE_LEFT_HANDED", "GLM_FORCE_DEPTH_ZERO_TO_ONE")
        add_defines(module_name:upper().."_API=__declspec(dllexport)")

        -- enable and generate reflection
        if enable_reflection then
            add_deps('header_tool')
            add_deps('reflection') 
            set_policy('build.fence', true)
            add_rules("generated_cpp")

            -- add headers to check
            for _, file in pairs(os.files("**.hpp")) do
                add_files(file)
            end

            on_config(function (target)
                target:add("includedirs", target:autogendir().."/public/", { public = true })
            end)

            before_build(function (target)
                if (DEBUG) then
                    print("xmake run header_tool "..target:scriptdir().." $(buildir)/reflection/"..module_name)
                end
                os.mkdir(target:autogendir().."/private/")
                os.mkdir(target:autogendir().."/public/")
                --os.exec("$(buildir)/$(plat)/$(arch)/$(mode)/header_tool "..target:scriptdir().." "..target:autogendir().."/")
                -- will register the generated files in the link process, so we still needs to generate the .obj in a custom rule
                for _, file in pairs(os.files(target:autogendir().."/private/**.cpp")) do
                    target:add("files", file)
                end
            end)
        end

        -- search for files
        for _, file in pairs(os.files("private/**.cpp")) do
            add_files(file)
        end
        for _, file in pairs(os.files("**.hpp")) do
            add_headerfiles(file)
        end
        
        -- set include dirs
        add_includedirs("private", { public = false })
        if not is_executable then
            add_includedirs("public", { public = true })
        end

        -- add deps
        if deps then
            if DEBUG then
                print(table.unpack({ "\t-- dependencies :", table.unpack(deps) }))
            end
            add_deps(table.unpack(deps))
        end

        -- add packages
        if packages then
            packages_name = "";
            for _, package in ipairs(packages) do
                if type(package) == "table" then
                    if not BUILD_MONOLITHIC then
                        add_packages(package.name, {public = true})
                    else
                        add_packages(package.name, {public = package.public})
                    end
                    packages_name = packages_name..", "..package.name
                else
                    if not BUILD_MONOLITHIC then
                        add_packages(package, {public = true})
                    else
                        add_packages(package)
                    end
                    packages_name = packages_name..", "..package
                end
            end
            if DEBUG then
                print(table.unpack({ "\t-- packages :", packages_name}))
            end
        end
        
        -- set kind
        if is_executable then
            set_kind("binary")
        elseif BUILD_MONOLITHIC or not allow_shared_build then
            set_kind("static")
        else
            add_rules("utils.symbols.export_all", {export_classes = true})
            set_kind("shared")
        end
    end)
end


target("data", function(target)
    set_kind("object")
    for _, file in pairs(os.files("resources/**")) do
        add_extrafiles(file)
    end
end)

if DEBUG then
    print("################ building modules ################")
end

includes("src/**.lua");
if has_config("build-tests") then
    includes("tests/**.lua")
end

option_end()
    