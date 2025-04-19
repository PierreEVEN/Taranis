declare_module("module_test",
    {
        deps = {"module_manager", "types"},
        packages = {},
        is_executable = true,
        enable_reflection = true
    }
)

target("module_manager_test")
set_group("test")