declare_module(
    "gfx", 
    {"types", "job-sys"}, 
    {
        {name = "vulkan-loader", public = true},
        {name = "vulkan-memory-allocator", public = true},
        {name = "glm", public = true},
        "glfw",
        "imgui",
        "imgui docking"
    }, 
    false, 
    false
)

target("gfx")
set_group("engine")