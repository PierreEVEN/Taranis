declare_module(
    "gfx", 
    {"types"}, 
    {
        {name = "vulkan-loader", public = true},
        {name = "vulkan-memory-allocator", public = true},
        {name = "glm", public = true},
        "directxshadercompiler", 
        "imgui", "spirv-reflect",
        "glfw",
        "imgui docking"
    }, 
    false, 
    false
)

target("gfx")
set_group("engine")