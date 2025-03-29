scriptName SkyScript hidden

; Get current version of the SkyScript SKSE plugin installed
string function GetVersion() global native

; Returns true if the SkyScript SKSE plugin is installed
; Returns false if the SkyScript SKSE plugin is not installed
bool function IsInstalled() global
    return GetVersion()
endFunction

; Returns a list of available language backends
string[] function GetAvailableLanguageBackends() global native

; Execute a SkyScript script (optionally specifying a language backend)
; Returns the result of the script execution
string function Execute(string script, string language = "") global native
