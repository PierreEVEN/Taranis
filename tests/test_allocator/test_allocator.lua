declare_module(
    "test_allocator", 
    {
        deps = {"types"},
        is_executable = true,
        enable_reflection = true
    }
)

target("test_allocator")
    set_group("test")