add_rules("mode.debug", "mode.release")

set_languages("c++23")

-- local vs_runtime = is_mode("debug") and "MTd" or "MT"
-- set_runtimes(vs_runtime)

includes("xmake/*.lua")

add_repositories("SkyrimScripting     https://github.com/SkyrimScripting/Packages.git")
add_repositories("SkyrimScriptingBeta https://github.com/SkyrimScriptingBeta/Packages.git")
add_repositories("MrowrLib            https://github.com/MrowrLib/Packages.git")


mod_info = {
    name = "SkyScript",
    version = "0.0.1",
    author = "Mrowr Purr",
    email = "mrowr.purr@gmail.com",
    mod_files = {"Scripts"}
}

skyrim_versions = {"ae"}

includes("nodejs.lua")
includes("experiments/nodejs_example/xmake.lua")
-- includes("experiments/duktape_example/xmake.lua")
-- includes("experiments/**/xmake.lua")
-- includes("skse.lua")
-- includes("papyrus.lua")
