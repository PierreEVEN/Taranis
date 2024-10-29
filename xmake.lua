set_project("Ashwga")

DEBUG = true;
BUILD_MONOLITHIC = true;

function declare_module(module_name, deps, packages, is_executable)
    if DEBUG then
        print("### "..module_name.." ###")
    end
    target(module_name)
        if is_executable then
            set_kind("binary")
        elseif BUILD_MONOLITHIC then
            set_kind("static")
        else
            set_kind("shared")
        end

    add_includedirs("private", { public = false })
    add_includedirs("public", { public = true })
    for _, ext in ipairs({ ".c", ".cpp" }) do
        add_files("private/**.cpp")
    end

    for _, ext in ipairs({ ".h", ".hpp", ".inl", ".natvis" }) do
        add_headerfiles("public/(**" .. ext .. ")")
        add_headerfiles("private/(**" .. ext .. ")")
    end

    if deps then
        if DEBUG then
            print(table.unpack({ "\t-- dependencies :", table.unpack(deps) }))
        end
        add_deps(table.unpack(deps))
    end
    if packages then
        if DEBUG then
            print(table.unpack({ "\t-- packages :", table.unpack(packages) }))
        end
        add_packages(table.unpack(packages))
    end
    target_end()
end

add_defines("ENABLE_VALIDATION_LAYER")
if DEBUG then
    print("################ building modules ################")
end

includes("src/**.lua");