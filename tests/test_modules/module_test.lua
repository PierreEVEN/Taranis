declare_module("test_modules",
    {
        deps = {"module_manager", "types", "reflection"},
        packages = {},
        is_executable = true,
        enable_reflection = false
    }
)

target("test_modules")
set_group("test")