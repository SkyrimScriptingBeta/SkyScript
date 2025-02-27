#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include "NodejsEnvironment.h"

std::unique_ptr<Nodejs::NodejsEnvironment> nodejs_environment = nullptr;

inline void
CallMeFromPapyrus(RE::BSScript::Internal::VirtualMachine*, RE::VMStackID, RE::StaticFunctionTag*) {
    RE::ConsoleLog::GetSingleton()->Print("Hello, from C++ that Papyrus called.");
}

inline void
MakeNodeJsEnvironment(RE::BSScript::Internal::VirtualMachine*, RE::VMStackID, RE::StaticFunctionTag*) {
    RE::ConsoleLog::GetSingleton()->Print("Creating Node.js environment...");
    if (nodejs_environment) {
        RE::ConsoleLog::GetSingleton()->Print("Node.js environment already exists.");
        return;
    }

    nodejs_environment = std::make_unique<Nodejs::NodejsEnvironment>();

    RE::ConsoleLog::GetSingleton()->Print("Node.js environment created.");
}

inline void EvalJs(
    RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag*,
    std::string_view a_code
) {
    if (!nodejs_environment) {
        RE::ConsoleLog::GetSingleton()->Print("Node.js environment not created.");
        return;
    }

    RE::ConsoleLog::GetSingleton()->Print("Evaluating JavaScript code...");
    RE::ConsoleLog::GetSingleton()->Print(a_code.data());

    auto result = nodejs_environment->eval(a_code);

    if (result.IsEmpty()) {
        RE::ConsoleLog::GetSingleton()->Print("Failed to evaluate JavaScript code.");
        return;
    } else {
        RE::ConsoleLog::GetSingleton()->Print("JavaScript code evaluated successfully.");
    }
}

extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse) {
    SKSE::Init(a_skse);

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded)
            RE::ConsoleLog::GetSingleton()->Print("Hello, from another plugin.");
    });

    SKSE::GetPapyrusInterface()->Register([](RE::BSScript::IVirtualMachine* vm) {
        vm->RegisterFunction("CallMeFromPapyrus", "SomeKeyboardBinds", CallMeFromPapyrus);
        vm->RegisterFunction("MakeNodeJsEnvironment", "SomeKeyboardBinds", MakeNodeJsEnvironment);
        vm->RegisterFunction("EvalJs", "SomeKeyboardBinds", EvalJs);
        return true;
    });

    return true;
}