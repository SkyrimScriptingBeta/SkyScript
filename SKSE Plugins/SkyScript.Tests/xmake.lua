skse_plugin({
    name = "SkyScript.Tests",
    src = {"*.cpp"},
    packages = {"SkyrimScripting.Plugin"},
    -- mod_files = {"Scripts/*/*SkyScriptTest*"},
    mod_files = {"Scripts"}
})