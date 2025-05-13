declare_module(
    "types",
    {
        deps = {"reflection", "io"},
        packages = {
            {name = "unordered_dense", public = true},
            {name = "glm", public = true}
        },
    }
)

target("types")
    set_group("utils")