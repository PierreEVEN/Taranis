declare_module("test_modules",
    {
        deps = {"module_manager", "types"},
        packages = {},
        is_executable = true,
        enable_reflection = true
    }
)

target("test_modules")
set_group("test")