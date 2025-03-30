#pragma once

namespace SkyScript {

    struct ISkyScriptBackend {
        virtual const char* name() const                       = 0;
        virtual const char* version() const                    = 0;
        virtual const char* description() const                = 0;
        virtual const char* programming_language() const       = 0;
        virtual const char* execute_script(const char* script) = 0;  // this WILL have context etc

    protected:
        virtual ~ISkyScriptBackend() = 0;
    };
}
