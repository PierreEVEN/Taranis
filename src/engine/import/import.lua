declare_module(
    "import", 
    {"core"}, 
    {"stb", "assimp"}, 
    false, 
    false
)

target("import")
set_group("engine")