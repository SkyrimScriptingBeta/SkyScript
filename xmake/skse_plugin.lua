function skse_plugin(plugin_info)
    local commonlib_version = get_config("commonlib"):match("skyrim%-commonlib%-(.*)")
    local mods_folders = os.getenv("SKYRIM_MODS_FOLDERS")

    if mods_folders then
        plugin_info.mods_folders = mods_folders
    else
        print("SKYRIM_MODS_FOLDERS environment variable not set")
    end
    
    target((plugin_info.target or plugin_info.name) .. " (" .. commonlib_version:upper() .. ")")
        set_basename(plugin_info.name .. "-" .. commonlib_version:upper())
        add_packages("skyrim-commonlib-" .. commonlib_version)
        add_rules("@skyrim-commonlib-" .. commonlib_version .. "/plugin", {
            mod_name = plugin_info.name .. " (" .. commonlib_version:upper() .. ")",
            mods_folders = plugin_info.mods_folders or "",
            mod_files = plugin_info.mod_files,
            name = plugin_info.name,
            version = plugin_info.version,
            author = plugin_info.author,
            email = plugin_info.email
        })
        for _, src in ipairs(plugin_info.src or {}) do
            add_files(src)
        end
        if plugin_info.include then
            add_includedirs(plugin_info.include)
        end
        for _, package in ipairs(plugin_info.packages or {}) do
            add_packages(package)
        end
        for _, dependency in ipairs(plugin_info.deps or {}) do
            add_deps(dependency)
        end
        for _, define in ipairs(plugin_info.defines or {}) do
            add_defines(define)
        end
        for _, link in ipairs(plugin_info.links or {}) do
            add_links(link)
        end
        add_deps("Build Papyrus Scripts")
end
