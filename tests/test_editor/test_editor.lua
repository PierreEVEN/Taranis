declare_module("test_editor", {"core", "types", "import"}, {"vulkan-loader", "vulkan-memory-allocator", "glfw", "imgui"}, true, true)

target("test_editor")
set_group("test")