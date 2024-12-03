declare_module("types", {"reflection"}, {
    {name = "unordered_dense", public = true},
    {name = "glm", public = true}
}, false, false)

target("types")
set_group("engine")