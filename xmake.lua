-- add_rules("mode.debug", "mode.release")
add_rules("mode.debug")

-- local vs_runtime = is_mode("debug") and "MTd" or "MT"
-- set_runtimes(vs_runtime)

add_repositories("SkyrimScripting https://github.com/SkyrimScripting/Packages.git")
add_repositories("MrowrLib https://github.com/MrowrLib/Packages.git")

set_languages("c++23")

mod_info = {
    name = "SkyScript",
    version = "0.0.1",
    author = "Mrowr Purr",
    email = "mrowr.purr@gmail.com",
    mod_files = {"Scripts"}
}

skyrim_versions = {"ae"}

includes("experiments/**/xmake.lua")
includes("skse.lua")
includes("papyrus.lua")
