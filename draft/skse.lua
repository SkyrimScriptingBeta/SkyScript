-- https://github.com/xmake-io/xmake/issues/1071


-- add_rules("mode.debug", "mode.release")

-- local vs_runtime = is_mode("debug") and "MTd" or "MT"
-- set_runtimes(vs_runtime)

-- This is expected:
--     warning: add_requires(fmt): vs_runtime is deprecated, please use runtimes!
add_requires("fmt", { configs = { header_only = false, vs_runtime = vs_runtime } })
add_requires("spdlog", { configs = { header_only = false, fmt_external = true, vs_runtime = vs_runtime } })
add_requires("_Log_", { configs = { vs_runtime = vs_runtime } })

mods_folders = os.getenv("SKYRIM_MODS_FOLDERS")

if mods_folders then
    mod_info.mods_folders = mods_folders
else
    print("SKYRIM_MODS_FOLDERS environment variable not set")
end

print("Skyrim versions to build for: " .. table.concat(skyrim_versions, ", "))

for _, game_version in ipairs(skyrim_versions) do
    add_requires("skyrim-commonlib-" .. game_version, { configs = { vs_runtime = vs_runtime } })
end

for _, game_version in ipairs(skyrim_versions) do
    target("SKSE_" .. game_version:upper())
        set_basename(mod_info.name .. "-" .. game_version:upper())
        add_files("src/*.cpp")
        add_includedirs("include")
        add_packages("_Log_")
        add_packages("skyrim-commonlib-" .. game_version)
        add_rules("@skyrim-commonlib-" .. game_version .. "/plugin", {
            mod_name = mod_info.name .. " (" .. game_version:upper() .. ")",
            mods_folders = mod_info.mods_folders or "",
            mod_files = mod_info.mod_files,
            name = mod_info.name,
            version = mod_info.version,
            author = mod_info.author,
            email = mod_info.email
        })
        link_to_nodejs()
end
