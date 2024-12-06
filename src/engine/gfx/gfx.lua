declare_module(
    "gfx", 
    {"types", "job-sys", "shader_compiler", "gfx_types"}, 
    {
        {name = "vulkan-loader", public = true},
        {name = "glm", public = true},
        "glfw",
        "vulkan-memory-allocator",
        {name = "imgui", public = true},
        {name = "unordered_dense", public = true}
    }, 
    false, 
    false
)

target("gfx")
set_group("engine")