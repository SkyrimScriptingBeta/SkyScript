#include <unordered_set>

#include "NodejsEnvironment.h"

namespace Nodejs {

    using namespace v8;

    // We'll define a function callback to return for missing properties:
    void MyInterceptedFunction(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();

        // We'll just print out that this function was invoked
        LogText("[C++] Intercepted function called!");

        // Optionally, return something to JavaScript
        args.GetReturnValue().Set(
            String::NewFromUtf8(isolate, "Intercepted result").ToLocalChecked()
        );
    }

    static const std::unordered_set<std::string> s_prototypeProps = {
        "toString", "__proto__", "constructor", "valueOf",
        // etc.
    };

    static std::unordered_map<std::string, std::string> g_staticProperties = {
        {"myString", "This is a string!"},
        // Add other static properties here
    };

    void MyNamedPropertyGetter(
        v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info
    ) {
        LogText("[C++] Intercepting named property access...");
        v8::Isolate*    isolate = info.GetIsolate();
        v8::HandleScope handle_scope(isolate);

        // Get the current context from the isolate
        v8::Local<v8::Context> context = isolate->GetCurrentContext();

        // Important: Add a context scope to ensure values are created in the right context
        v8::Context::Scope context_scope(context);

        // Convert property name to a string for debugging
        v8::String::Utf8Value utf8(isolate, property);
        std::string           propName = *utf8 ? *utf8 : "<unknown>";
        LogText(fmt::format("[C++] Intercepted property access: '{}'", propName).c_str());

        // Get the object being accessed
        v8::Local<v8::Object> holder = info.Holder();

        // Then modify your property handling:
        if (g_staticProperties.count(propName)) {
            LogText(fmt::format("[C++] Returning static property: '{}'", propName).c_str());
            if (propName == "myString") {
                v8::Local<v8::String> strVal =
                    v8::String::NewFromUtf8(isolate, g_staticProperties[propName].c_str())
                        .ToLocalChecked();
                info.GetReturnValue().Set(strVal);
            }
            return;
        }

        if (s_prototypeProps.count(propName)) {
            // Return the real property or do nothing
            auto realVal = holder->GetRealNamedProperty(context, property);
            if (!realVal.IsEmpty()) {
                info.GetReturnValue().Set(realVal.ToLocalChecked());
            }
            return;
        }

        // 1) Check if a real (non-interceptor) property already exists
        bool hasReal = false;
        if (!holder->HasRealNamedProperty(context, property).To(&hasReal)) {
            // If To() fails, something else is off, just return
            return;
        }

        if (propName == "nativeLog" || propName == "globalThis" || propName == "console" ||
            propName == "sandbox" || propName == "Proxy" || propName == "myProxy") {
            // Immediately get the real property or do nothing
            // so we don't override or cause recursion
            v8::MaybeLocal<v8::Value> realVal = holder->GetRealNamedProperty(context, property);
            if (!realVal.IsEmpty()) {
                info.GetReturnValue().Set(realVal.ToLocalChecked());
            }
            return;
        }

        if (hasReal) {
            // 2a) If it *does* exist, return that real property so we don't override it
            v8::MaybeLocal<v8::Value> realVal = holder->GetRealNamedProperty(context, property);
            if (!realVal.IsEmpty()) {
                info.GetReturnValue().Set(realVal.ToLocalChecked());
            }
            LogText("[C++] Real property found, returning it.");
            return;
        }

        // Safe implementation for returning different types
        try {
            if (propName == "myString") {
                // Create the string value
                v8::Local<v8::String> strVal =
                    v8::String::NewFromUtf8(isolate, "This is a string!").ToLocalChecked();

                // Create property descriptor with correct attributes
                v8::PropertyDescriptor desc(strVal);
                desc.set_enumerable(true);
                desc.set_configurable(true);

                // Use DefineProperty with this descriptor instead
                // The false at the end explicitly says "don't use interceptors"
                bool success = holder->DefineProperty(context, property, desc).FromMaybe(false);
                if (success) {
                    LogText("[C++] Successfully defined myString property");
                } else {
                    LogText("[C++] Failed to define myString property");
                }

                // Then return the value
                info.GetReturnValue().Set(strVal);
                return;
            } else if (propName == "myInteger") {
                LogText("[C++] Creating integer value...");
                // Use Integer instead of Number for integers
                info.GetReturnValue().Set(v8::Integer::New(isolate, 12345));
                LogText("[C++] Integer value returned");
                return;
            } else if (propName == "myFloat") {
                LogText("[C++] Creating float value...");
                info.GetReturnValue().Set(v8::Number::New(isolate, 69.420));
                LogText("[C++] Float value returned");
                return;
            } else if (propName == "myBoolean") {
                LogText("[C++] Creating boolean value...");
                // Boolean values should be safe and simple
                info.GetReturnValue().Set(false);
                LogText("[C++] Boolean value returned");
                return;
            } else if (propName == "myUndefined") {
                LogText("[C++] Returning undefined...");
                info.GetReturnValue().SetUndefined();
                LogText("[C++] Undefined returned");
                return;
            }

            // Default case: Create a function
            LogText("[C++] Creating function for unknown property...");
            v8::Local<v8::FunctionTemplate> funcTemplate =
                v8::FunctionTemplate::New(isolate, MyInterceptedFunction);
            v8::MaybeLocal<v8::Function> maybeFunc = funcTemplate->GetFunction(context);

            if (!maybeFunc.IsEmpty()) {
                info.GetReturnValue().Set(maybeFunc.ToLocalChecked());
                LogText("[C++] Function successfully created and returned");
            } else {
                LogText("[C++] Failed to create function, returning undefined");
                info.GetReturnValue().SetUndefined();
            }
        } catch (const std::exception& e) {
            LogText(fmt::format("[C++] Exception in property getter: {}", e.what()).c_str());
            info.GetReturnValue().SetUndefined();
        } catch (...) {
            LogText("[C++] Unknown exception in property getter");
            info.GetReturnValue().SetUndefined();
        }
    }
}
