target("Experiment - Hello SKSE")
    add_files("*.cpp")
    add_packages("skyrim-commonlib-ae")
    add_rules("@skyrim-commonlib-ae/plugin", {
        name = "[Purr] Hello SKSE",
        version = "420.1.69",
        author = "Mrowr Purr",
        email = "mrowr.purr@gmail.com",
        mods_folder = os.getenv("SKYSCRIPT_MODS_FOLDER")
    })
