declare_module(
    "llp", 
    {
        packages = {
            {name = "unordered_dense", public = true}
        },
    }
)

target("llp")
    set_group("utils")