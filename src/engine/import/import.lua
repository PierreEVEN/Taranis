declare_module(
    "import", 
    {"core"}, 
    {"stb", "tinygltf"}, 
    false, 
    true
)

target("import")
set_group("engine")