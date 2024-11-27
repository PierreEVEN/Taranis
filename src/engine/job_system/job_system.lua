declare_module(
    "job-sys", 
    {"types"}, 
    {
        {name = "unordered_dense", public = true},
        {name = "concurrentqueue", public = true}
    },
    false, 
    false
)

target("job-sys")
set_group("engine")