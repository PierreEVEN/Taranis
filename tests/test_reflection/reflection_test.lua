declare_module("test_reflection",
    {
        deps = {"reflection"},
        packages = {},
        is_executable = true,
        enable_reflection = true
    }
)

target("test_reflection")
set_group("test")