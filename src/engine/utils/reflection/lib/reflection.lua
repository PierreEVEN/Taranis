declare_module(
    "reflection", 
    {
        packages = {
            {name = "unordered_dense", public = true}
        },
    }
)

target("reflection")
    set_group("utils")