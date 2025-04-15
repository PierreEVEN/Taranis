declare_module(
    "io",
    {
        deps = {}, 
        packages = {
            {name = "unordered_dense", public = true}
        },
    }
)

target("types")
    set_group("engine")