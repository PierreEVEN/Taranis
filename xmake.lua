
set_project("TaranisEngine")
set_languages("cxx20")
set_allowedarchs("windows|x64", "linux|x86_64")
set_warnings("allextra")
set_rundir(".")

includes("xmake/rules/**.lua")

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
option("profiler", { default = true })

------------[[ DEPENDENCIES ]]--------------
add_repositories("taranis-repo xmake")
add_requires("assimp v5.4.3", {configs = {shared = true, no_export = true}})
add_requires("concurrentqueue v1.0.4")
add_requires("freeimage 3.18.0", {configs = {rgb = true, shared = true}})
add_requires("glfw 3.4", {configs = {shared = true}})
add_requires("llp", {configs = {shared = true}})
add_requires("glm 1.0.1")
add_requires("imgui v1.91.8-docking")
add_requires("nativefiledialog-extended v1.2.1")
add_requires("slang v2025.8.1", {verify = false, configs = {slangc = true, slang_glslang = true}}) -- //@TODO Slangc is not required by the engine but fails to compile otherwise : https://github.com/shader-slang/slang/issues/6868)

package("slang", function(package)
    set_homepage("https://github.com/shader-slang/slang")
    set_description("Making it easier to work with shaders")
    set_license("MIT")

    add_urls("https://github.com/shader-slang/slang.git")

    add_versions("v2025.6.3", "b9300bae08a77df6ef2efe2b62de14a13b10b9a4")
    add_versions("v2024.1.18", "efdbb954c57b89362e390f955d45f90e59d66878")
    add_versions("v2024.1.17", "62b7219e715bd4c0f984bcd98c9767fb6422c78f")

    add_configs("shared", { description = "Build shared library", default = true, type = "boolean", readonly = true })
    add_configs("embed_stdlib_source", { description = "Embed stdlib source in the binary", default = true, type = "boolean" })
    add_configs("embed_stdlib", { description = "Build slang with an embedded version of the stdlib", default = false, type = "boolean" })
    add_configs("full_ir_validation", { description = "Enable full IR validation (SLOW!)", default = false, type = "boolean" })
    add_configs("gfx", { description = "Enable gfx targets", default = false, type = "boolean" })
    add_configs("slangd", { description = "Enable language server target", default = false, type = "boolean" })
    add_configs("slangc", { description = "Enable standalone compiler target", default = true, type = "boolean" })
    add_configs("slangrt", { description = "Enable runtime target", default = false, type = "boolean" })
    add_configs("slang_glslang", { description = "Enable glslang dependency and slang-glslang wrapper target", default = false, type = "boolean" })
    add_configs("slang_llvm_flavor", { description = "How to get or build slang-llvm (available options: FETCH_BINARY, USE_SYSTEM_LLVM, DISABLE)", default = "DISABLE", type = "string" })

    add_deps("cmake")

    on_install("windows|x64", "macosx", "linux|x86_64", function (package)
        io.replace("cmake/SlangTarget.cmake", [[set_property(TARGET ${target} PROPERTY SUFFIX ".dylib")]], "", {plain = true})
        local configs = {"-DSLANG_ENABLE_TESTS=OFF", "-DSLANG_ENABLE_EXAMPLES=OFF"}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:is_debug() and "Debug" or "Release"))
        table.insert(configs, "-DSLANG_LIB_TYPE=" .. (package:config("shared") and "SHARED" or "STATIC"))
        table.insert(configs, "-DSLANG_EMBED_STDLIB_SOURCE=" .. (package:config("embed_stdlib_source") and "ON" or "OFF"))
        table.insert(configs, "-DSLANG_EMBED_STDLIB=" .. (package:config("embed_stdlib") and "ON" or "OFF"))
        table.insert(configs, "-DSLANG_ENABLE_FULL_IR_VALIDATION=" .. (package:config("full_ir_validation") and "ON" or "OFF"))
        table.insert(configs, "-DSLANG_ENABLE_ASAN=" .. (package:config("asan") and "ON" or "OFF"))
        table.insert(configs, "-DSLANG_ENABLE_GFX=" .. (package:config("gfx") and "ON" or "OFF"))
        table.insert(configs, "-DSLANG_ENABLE_SLANGD=" .. (package:config("slangd") and "ON" or "OFF"))
        table.insert(configs, "-DSLANG_ENABLE_SLANGC=" .. (package:config("slangc") and "ON" or "OFF"))
        table.insert(configs, "-DSLANG_ENABLE_SLANGRT=" .. (package:config("slangrt") and "ON" or "OFF"))
        table.insert(configs, "-DSLANG_ENABLE_SLANG_GLSLANG=" .. (package:config("slang_glslang") and "ON" or "OFF"))
        table.insert(configs, "-DSLANG_SLANG_LLVM_FLAVOR=" .. package:config("slang_llvm_flavor"))

        import("package.tools.cmake").install(package, configs)
        package:addenv("PATH", "bin")
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({ test = [[
            #include <slang-com-ptr.h>
            #include <slang.h>

            void test() {
                Slang::ComPtr<slang::IGlobalSession> global_session;
                slang::createGlobalSession(global_session.writeRef());
            }
        ]] }, {configs = {languages = "c++17"}}))
    end)

    add_patches("v2025.8.1", path.join(os.projectdir(), "xmake/patches/slang/v2025.8.1/fix_std-nullptr_t.patch"))
end)
--add_requires("slang-fix v2025.8.1", {verify = false, configs = {slangc = true, slang_glslang = true}}) -- //@TODO Slangc is not required by the engine but fails to compile otherwise : https://github.com/shader-slang/slang/issues/6868)
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
    
    target(module_name, function ()

	    add_cxxflags("-Wno-invalid-offsetof", {tools = {"gcc", "clang"}})
	    add_cxxflags("-Wno-missing-field-initializers", {tools = "gcc"})

        if has_config("profiler") then
            add_defines("ENABLE_PROFILER")
        end

        add_defines("GLM_FORCE_LEFT_HANDED", "GLM_FORCE_DEPTH_ZERO_TO_ONE")
        --add_defines(module_name:upper().."_API=__declspec(dllexport)")

        -----------------[[ REFLECTION ]]-----------------
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

        -----------------[[ SOURCES ]]-----------------
        cpp_files = os.files("private/**.cpp")
        for _, file in pairs(cpp_files) do
            add_files(file)
        end

        -----------------[[ HEADERS ]]-----------------
        if os.exists("public") then
            add_includedirs("public", { public = not is_executable })
            for _, file in pairs(os.files("public/**.hpp")) do
                add_headerfiles(file)
            end
        end
        if os.exists("private") then
            add_includedirs("private", { public = false })
            for _, file in pairs(os.files("private/**.hpp")) do
                add_headerfiles(file)
            end
        end

        -----------------[[ TESTS ]]-----------------
        if os.exists("tests") then
            for _, file in pairs(os.files("tests/test_**.cpp")) do
                local name = path.basename(file)
                target("test_" .. module_name .. "_" .. name, function ()
                    set_kind("binary")
                    add_includedirs("tests", { public = false })
                    add_deps(module_name)
                    set_default(false)
                    add_files(file)
                    add_tests("default")

                    -- Reflection : //@TODO : make reflection optional
                    add_cxxflags("-Wno-invalid-offsetof", {tools = {"gcc", "clang"}})
                    add_deps('header_tool')
                    set_policy('build.fence', true)
                    add_rules("header.tool.generated")
                    add_deps('reflection')

                    -- add headers to check
                    for _, file in pairs(os.files("tests/**.hpp")) do
                        add_files(file)
                    end
                end)
            end
        end

        -----------------[[ REQUIRED MODULES ]]-----------------
        if deps then
            add_deps(table.unpack(deps))
        end

        -----------------[[ LIBRARIES ]]-----------------
        local build_monolithic = has_config("build-tests")
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

        -----------------[[ SET TYPE ]]-----------------
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

includes("src/**.lua");