declare_module(
    "gfx", 
    {"types", "job-sys", "shader_compiler", "gfx_types"}, 
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