#pragma once

#include "ISkyScriptBackendsManager.h"

namespace SkyScript {

    struct ISkyScriptService {
        virtual ISkyScriptBackendsManager* backends() = 0;

    protected:
        virtual ~ISkyScriptService() = 0;
    };
}
