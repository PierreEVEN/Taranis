declare_module(
    "import", 
    {"core"}, 
    {
        "freeimage",
        "assimp"
    }, 
    false, 
    false
)

target("import")
set_group("engine")