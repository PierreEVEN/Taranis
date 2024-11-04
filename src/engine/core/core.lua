declare_module("core", {"types"}, {"vulkan-loader", "directxshadercompiler", "imgui", "stb", "vulkan-validationlayers", "vulkan-memory-allocator", "spirv-reflect", "glfw", {name = "glm", public = true}, "imgui docking", "stb", "tinygltf"}, false)

target("core")
set_group("engine")