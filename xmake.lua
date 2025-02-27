-- https://github.com/xmake-io/xmake/issues/1071

add_rules("mode.debug", "mode.release")

local vs_runtime = is_mode("debug") and "MTd" or "MT"

set_runtimes(vs_runtime)

set_languages("c++23")

add_repositories("SkyrimScripting https://github.com/SkyrimScripting/Packages.git")

-- This is expected:
-- warning: add_requires(fmt): vs_runtime is deprecated, please use runtimes!

add_requires("fmt", { configs = { header_only = false, vs_runtime = vs_runtime } })
add_requires("spdlog", { configs = { header_only = false, fmt_external = true, vs_runtime = vs_runtime } })
add_requires("skyrim-commonlib-ae", { configs = { vs_runtime = vs_runtime } })

includes("xmake/*.lua")
includes("mods/**/xmake.lua")
