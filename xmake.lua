
set_project("TaranisEngine")
set_languages("cxx20")
set_allowedarchs("windows|x64", "linux|x86_64")
set_warnings("allextra")
set_rundir(".")
--add_rules("plugin.vsxmake.autoupdate") // trigger to often : wait improvements for the vsxmake generator

------------[[ MODES ]]--------------

add_rules("mode.debug", "mode.release")
set_allowedmodes("debug", "release")
set_defaultmode("release")
if is_mode("debug") then
    set_optimize("fastest")
end
if is_mode("release") then
    set_symbols("hidden")
    set_optimize("fastest")
    set_strip("all")
end
if is_plat("windows") then
    set_runtimes(is_mode("debug") and "MDd" or "MD")
end

------------[[ OPTIONS ]]--------------

option("build-monolithis", { default = false })
option("build-tests", { default = true })
option("profiler", { default = true })

------------[[ DEPENDENCIES ]]--------------

add_requires("assimp v5.4.3", {configs = {shared = true, no_export = true}})
add_requires("concurrentqueue v1.0.4")
add_requires("freeimage 3.18.0", {configs = {rgb = true, shared = true}})
add_requires("glfw 3.4", {configs = {shared = true}})
add_requires("glm 1.0.1")
add_requires("imgui v1.91.8-docking")
add_requires("nativefiledialog-extended v1.2.1")
add_requires("slang v2025.8.1", {verify = false, configs = {slangc = true, slang_glslang = true}}) -- //@TODO Slangc is not required by the engine but fails to compile otherwise : https://github.com/shader-slang/slang/issues/6868)
add_requires("unordered_dense v4.5.0")
add_requires("vulkan-loader")
add_requires("vulkan-memory-allocator v3.2.1")
add_requires("nlohmann_json v3.11.3")

function declare_module(module_name, opts)

    if (opts == nil) then
        print("Error : invalid options for module "..module_name)
        os.exit(-1)
    end
    
    local deps = opts.deps or {}
    local packages = opts.packages or {}
    local is_executable = opts.is_executable or false
    local is_module = opts.is_module or false
    local enable_reflection = opts.enable_reflection or false
    local allow_shared_build = opts.allow_shared_build or false
    
    target(module_name, function (current_target)

	    add_cxxflags("-Wno-invalid-offsetof", {tools = "gcc"})
	    add_cxxflags("-Wno-missing-field-initializers", {tools = "gcc"})

        if has_config("profiler") then
            add_defines("ENABLE_PROFILER")
        end

        add_defines("GLM_FORCE_LEFT_HANDED", "GLM_FORCE_DEPTH_ZERO_TO_ONE")
        --add_defines(module_name:upper().."_API=__declspec(dllexport)")

        -- enable and generate reflection
        if enable_reflection then
            add_deps('header_tool')
            set_policy('build.fence', true)
            add_rules("header.tool.generated")
            add_deps('reflection')

            -- add headers to check
            for _, file in pairs(os.files("**.hpp")) do
                add_files(file)
            end
        end

        -- search for files
        cpp_files = os.files("private/**.cpp")
        for _, file in pairs(cpp_files) do
            add_files(file)
        end

        for _, file in pairs(os.files("public/**.hpp")) do
            add_headerfiles(file)
        end
        if os.exists("private") then
            add_includedirs("private", { public = false })
            for _, file in pairs(os.files("private/**.hpp")) do
                add_headerfiles(file)
            end
        end

        -- set include dirs
        if not is_executable then
            add_includedirs("public", { public = true })
        end

        -- add deps
        if deps then
            add_deps(table.unpack(deps))
        end

        local build_monolithic = has_config("build-tests")

        -- add packages
        if packages then
            packages_name = "";
            for _, package in ipairs(packages) do
                if type(package) == "table" then
                    if not build_monolithic then
                        add_packages(package.name, {public = true})
                    else
                        add_packages(package.name, {public = package.public})
                    end
                    packages_name = packages_name..", "..package.name
                else
                    if not build_monolithic then
                        add_packages(package, {public = true})
                    else
                        add_packages(package)
                    end
                    packages_name = packages_name..", "..package
                end
            end
        end

        -- set kind
        if is_executable then
            set_kind("binary")
        elseif #cpp_files == 0 then
            set_kind("headeronly")
        elseif build_monolithic or (not allow_shared_build and not is_module) then
            set_kind("static")
        else
            if not is_module then
                add_rules("utils.symbols.export_all", {export_classes = true})
            end
            set_kind("shared")
        end
    end)
end

-- So resource folder will be available within Visual Studio
target("data", function(target)
    set_kind("phony")
    for _, file in pairs(os.files("resources/**")) do
        add_extrafiles(file)
    end
end)

includes("xmake/**.lua");
includes("src/**.lua");
if has_config("build-tests") then
    includes("tests/**.lua")
end