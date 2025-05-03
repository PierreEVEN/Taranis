declare_module(
    "header_scanner", 
    
    {
        packages = {"nlohmann_json"},
        is_executable = true,
    }
)

target("header_scanner")
    set_group("utils")
    set_policy('build.fence', true)