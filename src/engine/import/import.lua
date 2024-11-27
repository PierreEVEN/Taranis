declare_module(
    "import", 
    {"core"}, 
    {
        "freeimage",
        "assimp",
        {name = "unordered_dense", public = true}
    }, 
    false, 
    false
)

target("import")
set_group("engine")