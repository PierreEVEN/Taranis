add_rules("mode.debug", "mode.release")
add_rules("plugin.vsxmake.autoupdate")

set_project("Ashwga")
set_languages("clatest", "cxx20")
set_allowedarchs("windows|x64")
set_warnings("allextra")
set_allowedmodes("debug", "release")
set_defaultmode("release")
set_rundir(".")

DEBUG = false;
BUILD_MONOLITHIC = true;



set_runtimes(is_mode("debug") and "MTd" or "MT")
		
add_requires("vulkan-loader", "glfw", "glm", "imgui docking", "stb", "vulkan-validationlayers", "vulkan-memory-allocator", "directxshadercompiler", "spirv-reflect", "tinygltf")


function declare_module(module_name, deps, packages, is_executable, enable_reflection)
    if DEBUG then
        print("### "..module_name.." ###")
    end
    target(module_name)
        if is_executable then
            set_kind("binary")
        elseif BUILD_MONOLITHIC then
            set_kind("static")
        else
            set_kind("shared")
        end

    add_includedirs("private", { public = false })
    if not is_executable then
        add_includedirs("public", { public = true })
    end
    for _, ext in ipairs({ ".c", ".cpp" }) do
        add_files("private/**.cpp")
    end

    for _, ext in ipairs({ ".h", ".hpp", ".inl", ".natvis" }) do
        add_headerfiles("public/(**" .. ext .. ")")
        add_headerfiles("private/(**" .. ext .. ")")
    end

    if deps then
        if DEBUG then
            print(table.unpack({ "\t-- dependencies :", table.unpack(deps) }))
        end
        add_deps(table.unpack(deps))
    end
    if packages then
        packages_name = "";
        for _, package in ipairs(packages) do
            if type(package) == "table" then
                add_packages(package.name, {public = package.public})
                packages_name = packages_name..", "..package.name
            else
                add_packages(package)
                packages_name = packages_name..", "..package
            end
        end
        if DEBUG then
            print(table.unpack({ "\t-- packages :", packages_name}))
        end
    end

    if enable_reflection then
        add_deps('header_tool')
        before_build(function (target)
            os.mkdir("$(buildir)/reflection/"..module_name.."/private/")
            os.mkdir("$(buildir)/reflection/"..module_name.."/public/")
            os.exec("xmake run header_tool "..target:scriptdir().." $(buildir)/reflection/"..module_name)
        end)
        
        add_deps('reflection')
        add_files("$(buildir)/reflection/"..module_name.."/private/**.cpp")
        add_includedirs("$(buildir)/reflection/"..module_name.."/public/")
    end

    target_end()
end

add_defines("ENABLE_VALIDATION_LAYER")
if DEBUG then
    print("################ building modules ################")
end

includes("src/**.lua");