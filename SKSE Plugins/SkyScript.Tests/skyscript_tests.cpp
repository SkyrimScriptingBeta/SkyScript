#include <SkyrimScripting/Plugin.h>

bool TestOne(RE::StaticFunctionTag*) {
    Log("TestOne called");
    return true;
}

bool TestTwo(RE::StaticFunctionTag*) {
    Log("TestTwo called");
    return true;
}

bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("TestOne", "RunSkyScriptTests", TestOne);
    vm->RegisterFunction("TestTwo", "RunSkyScriptTests", TestTwo);
    return true;
}

SKSEPlugin_Entrypoint { SKSE::GetPapyrusInterface()->Register(RegisterPapyrusFunctions); }
