declare_module(
    "test_editor",
    {
        deps = {"core", "types", "import"},
        packages = {"glfw", "nativefiledialog-extended"},
        is_executable = true,
        enable_reflection = true
    }
)

target("test_editor")
    set_group("test")