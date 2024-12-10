declare_module(
    "types",
    {
        deps = {"reflection"}, 
        packages = {
            {name = "unordered_dense", public = true},
            {name = "glm", public = true}
        },
    }
)

target("types")
    set_group("engine")