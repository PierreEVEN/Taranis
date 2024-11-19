declare_module("test_app", {"core", "types", "import"}, {"vulkan-loader", "vulkan-memory-allocator", "glfw", "imgui"}, true, true)

target("test_app")
set_group("test")