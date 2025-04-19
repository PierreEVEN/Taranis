declare_module("example_module",
    {
        deps = {"module_manager", "types"},
        enable_reflection = true,
        is_module = true
    }
)

target("example_module")
set_group("test")