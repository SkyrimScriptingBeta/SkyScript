skse_plugin({
    name = "SkyScript",
    target = "SkyScript - SKSE Plugin",
    src = {"src/*.cpp"},
    include = {"include"},
    packages = {
        "SkyrimScripting.Plugin",
        "SkyrimScripting.Console",
        "SkyrimScripting.Services"
    },
    deps = {"SkyScript"},
    mod_files = {"Scripts"}
})