declare_module(
    "job-sys", 
    {
        deps = {"types"}, 
        packages = {
            {name = "unordered_dense", public = true},
            {name = "concurrentqueue", public = true}
        },
    }
)

target("job-sys")
set_group("engine")