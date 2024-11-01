declare_module("core", {"types"}, {"vulkan-loader", "vulkan-validationlayers",{name =  "vulkan-memory-allocator", public = true}, "glfw", {name = "glm", public = true}, "imgui docking", "stb"}, false)

target("core")
set_group("engine")