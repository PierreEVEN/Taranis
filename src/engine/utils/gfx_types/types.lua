declare_module(
    "gfx_types",
    {
        deps = {"types"}, 
        packages = {
            {name = "unordered_dense", public = true}
        },
    }
)

target("gfx_types")
    set_group("engine")