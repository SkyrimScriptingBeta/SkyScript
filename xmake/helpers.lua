function task(name)
    target(name)
    set_kind("phony")
end

function compile_papyrus_scripts()
    before_build(function (target)
        print("Building Papyrus Scripts")
        local skyrim_with_ck = os.getenv("SKYRIM_WITH_CK")
        if not skyrim_with_ck then
            print("SKYRIM_WITH_CK environment variable not set")
            return
        end
        local ppj_path = path.join(target:scriptdir(), "Papyrus.ppj")
        os.exec("pyro -i " .. ppj_path .. " --game-path \"" .. skyrim_with_ck .. "\"")
    end)
end

function copy_files_into_mod_folder(path_matchers)
    after_build(function (target)
        local mods_folder_path = os.getenv("SKYSCRIPT_MODS_FOLDER")
        local mod_name = target:name()

        -- If it starts with "Papyrus: " then remove that prefix
        local prefix = "Papyrus: "
        if string.find(mod_name, prefix, 1, true) == 1 then
            mod_name = string.sub(mod_name, #prefix + 1)
        end

        local mod_folder = path.join(mods_folder_path, mod_name)

        -- if the mod folder already exists, delete it
        if os.isdir(mod_folder) then
            os.rmdir(mod_folder)
        end

        print("Copying files to " .. mod_folder)
        os.mkdir(mod_folder)

        for _, path_matcher in ipairs(path_matchers) do
            -- make the path_matcher relative to the scriptdir
            path_matcher = path.join(target:scriptdir(), path_matcher)

            os.cp(path.join(path_matcher), mod_folder)
        end
    end)
end