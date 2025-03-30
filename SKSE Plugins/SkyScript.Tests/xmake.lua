skse_plugin({
    name = "SkyScript.Tests",
    src = {"*.cpp"},
    packages = {"SkyrimScripting.Plugin", "quickjs-ng"}
    -- mod_files = {"Scripts/*/*SkyScriptTest*"},
})