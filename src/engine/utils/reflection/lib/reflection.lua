declare_module(
    "reflection", 
    {
        deps = {"io"},
        enable_test_reflection = true,
        packages = {
            {name = "unordered_dense", public = true}
        },
    }
)

target("reflection")
    set_group("engine/utils")