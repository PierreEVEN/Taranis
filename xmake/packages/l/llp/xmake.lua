package("llp")
    set_homepage()
    set_description("Light Language Processor is a tiny helper library for parsing any data structure.")
    set_license("mit")

    add_urls("https://github.com/PierreEVEN/llp.git")
    add_deps("cmake")

    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:is_debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        import("package.tools.cmake").install(package, configs)
    end)