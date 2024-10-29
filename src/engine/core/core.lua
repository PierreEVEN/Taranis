declare_module("core", {"types"}, {"vulkan-loader", "glfw", {name = "glm", public = true}, "imgui docking", "stb", "vulkan-validationlayers", "vulkan-memory-allocator"}, false)

target("core")
set_group("engine")