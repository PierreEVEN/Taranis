declare_module(
    "editor",
    {
        deps = {"core", "types", "import"},
        packages = {"glfw", "nativefiledialog-extended"},
        is_executable = true,
        enable_reflection = true
    }
)

target("editor")
    set_group("editor")
    set_default(true)