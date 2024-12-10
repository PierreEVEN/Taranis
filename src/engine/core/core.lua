declare_module(
    "core", 
    {
        deps = {"types", "gfx", "job-sys"},
        packages = {{name = "unordered_dense", public = true}},
        enable_reflection = true
    }
)

target("core")
    if is_plat("windows") then
        add_syslinks("winmm")
    end
    set_group("engine")
