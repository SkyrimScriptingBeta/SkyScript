#include <SkyrimScripting/Plugin.h>

std::string GetVersion(RE::StaticFunctionTag*) { return "1.0.0"; }

std::vector<std::string> GetAvailableLanguageBackends(RE::StaticFunctionTag*) { return {"js"}; }

std::string Execute(
    RE::StaticFunctionTag*, std::string_view script, std::string_view language = ""
) {
    return "TODO";
}

bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("GetVersion", "SkyScript", GetVersion);
    vm->RegisterFunction("GetAvailableLanguageBackends", "SkyScript", GetAvailableLanguageBackends);
    vm->RegisterFunction("Execute", "SkyScript", Execute);
    return true;
}

SKSEPlugin_Entrypoint { SKSE::GetPapyrusInterface()->Register(RegisterPapyrusFunctions); }
