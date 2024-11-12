declare_module(
    "job-sys", 
    {"types"}, 
    {{name = "concurrentqueue", public = true}},
    false, 
    false
)

target("job-sys")
set_group("engine")