declare_module(
    "import", 
    {
        deps = {"core"}, 
        packages = {
            "freeimage",
            "assimp",
            {name = "unordered_dense", public = true}
        }
    }
)

target("import")
    set_group("engine")