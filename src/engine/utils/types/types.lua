declare_module(
    "types",
    {
        deps = {"reflection", "io"},
        enable_test_reflection = true,
        packages = {
            {name = "unordered_dense", public = true},
            {name = "glm", public = true}
        },
    }
)

target("types")
    set_group("engine/utils")