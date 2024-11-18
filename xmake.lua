add_rules("mode.debug", "mode.releasedbg", "mode.release")

set_languages("c++23")

add_repositories("SkyrimScripting https://github.com/SkyrimScripting/Packages.git")

add_requires("skyrim-commonlib")

includes("experiments/*/*/xmake.lua")
