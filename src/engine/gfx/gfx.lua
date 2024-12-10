declare_module(
    "gfx", 
    {
        deps = {"types", "job-sys", "shader_compiler", "gfx_types"}, 
        packages = {
            "glfw",
            "vulkan-memory-allocator",
            {name = "vulkan-loader", public = true},
            {name = "glm", public = true},
            {name = "imgui", public = true},
            {name = "unordered_dense", public = true}
        }
    }
)

target("gfx")
    set_group("engine")