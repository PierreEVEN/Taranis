declare_module(
    "shader_compiler",
    {
        deps = {"llp", "gfx_types"}, 
        packages = {
            "slang",
            "unordered_dense"
        },
    }
)

target("shader_compiler")
    set_group("utils")