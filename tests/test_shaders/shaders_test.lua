declare_module(
    "test_shaders",
    {
        deps = {"shader_compiler"},
        is_executable = true,
        enable_reflection = true
    }
)

target("test_shaders")
    set_group("test")