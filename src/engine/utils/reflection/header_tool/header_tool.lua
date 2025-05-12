declare_module(
    "header_tool", 
    
    {
        packages = {
            "llp",
            {name = "unordered_dense", public = true}
        },
        is_executable = true,
    }
)

target("header_tool")
    set_group("utils")
    set_policy('build.fence', true)