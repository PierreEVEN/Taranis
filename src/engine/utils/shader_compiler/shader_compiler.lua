declare_module(
    "shader_compiler",
    {
        deps = {"gfx_types"},
        packages = {
            "llp",
            "slang",
            "unordered_dense"
        },
    }
)

target("shader_compiler")
    set_group("utils")
    if is_plat("linux") then
        add_syslinks("pthread")
    end