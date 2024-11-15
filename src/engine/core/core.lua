declare_module(
    "core", 
    {"types", "gfx", "job-sys"}, 
    {}, 
    false, 
    true
)

target("core")
if is_plat("windows") then
    add_syslinks("winmm")
end
set_group("engine")
