declare_module(
    "module_manager",
    {
        packages = {
            {name = "unordered_dense", public = true}
        },
        enable_reflection = true
    }
)

target("module_manager")
    set_group("engine")