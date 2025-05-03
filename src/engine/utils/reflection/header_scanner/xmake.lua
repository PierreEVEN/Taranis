declare_module(
    "header_scanner", 
    
    {
        packages = {"nlohmann_json", "unordered_dense"},
        is_executable = true,
    }
)

target("header_scanner")
    set_group("utils")
    set_policy('build.fence', true)