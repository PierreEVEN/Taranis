declare_module("core", {"types"}, {"vulkan-loader", "directxshadercompiler", "vulkan-validationlayers", "vulkan-memory-allocator", "glfw", {name = "glm", public = true}, "imgui docking", "stb"}, false)

target("core")
set_group("engine")