declare_module("header_tool", {"llp"}, {{name = "unordered_dense", public = true}}, true, false)

target("header_tool")
set_group("utils")
set_policy('build.fence', true)