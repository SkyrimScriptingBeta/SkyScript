#include <string>
#include <string_view>

#include "../Interfaces.h"

namespace SkyScript {

    class SkyScriptBackendBase : public ISkyScriptBackend {
        std::string _name;
        std::string _version;
        std::string _description;
        std::string _programming_language;

    public:
        SkyScriptBackendBase(
            std::string_view name, std::string_view version, std::string_view description,
            std::string_view programming_language
        )
            : _name(name),
              _version(version),
              _description(description),
              _programming_language(programming_language) {}

        ~SkyScriptBackendBase() = default;

        const char* name() const override { return _name.c_str(); }
        const char* version() const override { return _version.c_str(); }
        const char* description() const override { return _description.c_str(); }
        const char* programming_language() const override { return _programming_language.c_str(); }
    };
}