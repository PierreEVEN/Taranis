declare_module(
    "import", 
    {"core"}, 
    {"stb", "tinygltf"}, 
    false, 
    false
)

target("import")
set_group("engine")