#pragma once

#include "ISkyScriptBackend.h"

namespace SkyScript {

    struct ISkyScriptBackendsManager {
        virtual void               register_backend(ISkyScriptBackend* backend)   = 0;
        virtual void               unregister_backend(ISkyScriptBackend* backend) = 0;
        virtual ISkyScriptBackend* get_backend(const char* name)                  = 0;

    protected:
        virtual ~ISkyScriptBackendsManager() = 0;
    };
}
