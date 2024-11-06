declare_module(
    "core", 
    {"types", "gfx"}, 
    {"stb", "tinygltf"}, 
    false, 
    true
)

target("core")
set_group("engine")