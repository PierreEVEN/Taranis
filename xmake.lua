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
BUILD_MONOLITHIC = true;

option("build-tests", { default = false })

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

add_requires("vulkan-loader", "glm", "imgui docking", "vulkan-memory-allocator", "concurrentqueue", "unordered_dense")
add_requires("glfw", {configs = {shared = true}})
add_requires("assimp", {configs = {shared = true, no_export = true}})
add_requires("freeimage", {configs = {rgb = true, shared = true}})
add_requires("slang v2024.15.1", {verify = false})

rule("generated_cpp", function (rule)
    set_extensions(".hpp")
    before_buildcmd_file(function (target, batchcmds, source_header, opt)

        -- Guess generated source file path
        local generated_path = string.sub(os.projectdir().."/"..source_header, string.len(target:scriptdir()) + 2)
        -- replace .hpp extension with .gen.cpp
        local generated_source = target:autogendir().."/"..string.sub(generated_path, 1, string.len(generated_path) - 3).."gen.cpp"

        -- generated classes are always private
        generated_source = generated_source:gsub("public", "private", 1)
        
        if (os.exists(generated_source)) then
            local test_str = tostring(generated_source)
            local objectfile = target:objectfile(generated_source)
            --batchcmds:vrunv(TODO : make the header tool works for one header at a time)
            batchcmds:compile(generated_source, objectfile)

            batchcmds:show_progress(opt.progress, "${color.build.object}Compiling.reflection %s", generated_source)
            batchcmds:add_depfiles(source_header)

            batchcmds:set_depmtime(os.mtime(objectfile))
            batchcmds:set_depcache(target:dependfile(objectfile))
        end
    end)
end)

function declare_module(module_name, deps, packages, is_executable, enable_reflection)
    if DEBUG then
        print("### "..module_name.." ###")
    end
    target(module_name, function (target)
        add_defines("GLM_FORCE_LEFT_HANDED", "GLM_FORCE_DEPTH_ZERO_TO_ONE")
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
                os.exec("$(buildir)/$(plat)/$(arch)/$(mode)/header_tool "..target:scriptdir().." "..target:autogendir().."/")
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
        elseif BUILD_MONOLITHIC then
            set_kind("static")
        else
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
    