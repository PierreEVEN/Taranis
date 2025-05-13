declare_module(
    "io",
    {
        deps = {}, 
        packages = {
            {name = "unordered_dense", public = true}
        },
    }
)

target("io")
    set_group("utils")