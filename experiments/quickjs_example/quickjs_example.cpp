#include <signal.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <string>
#include <unordered_map>

#include "quickjs.h"

using namespace std;

// Store dynamically created globals
unordered_map<string, JSValue> global_vars;

// Flag to check if CTRL+C was pressed
volatile sig_atomic_t ctrl_c_pressed = 0;

// Signal handler for CTRL+C
void handle_signal(int signal) {
    if (signal == SIGINT) {
        ctrl_c_pressed = 1;
    }
}

// C++ function exposed to JS
JSValue js_lookup_global(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    cout << "C++ function called from JS" << endl;

    if (argc < 1 || !JS_IsString(argv[0])) {
        return JS_UNDEFINED;
    }

    // Get the global name
    size_t      len;
    const char* prop_name = JS_ToCStringLen(ctx, &len, argv[0]);

    if (!prop_name) {
        return JS_UNDEFINED;
    } else {
        cout << "Looking up global: " << prop_name << endl;
    }

    // Check if we already created this global
    auto it = global_vars.find(prop_name);
    if (it != global_vars.end()) {
        JS_FreeCString(ctx, prop_name);
        return JS_DupValue(ctx, it->second);
    }

    // If the prop name is "MyString" then lazily define a global string with the value "I am a
    // string!"
    if (strcmp(prop_name, "MyString") == 0) {
        JSValue new_global     = JS_NewString(ctx, "I am a string!");
        global_vars[prop_name] = JS_DupValue(ctx, new_global);

        // Define on globalThis so it persists
        JSValue global_obj = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, global_obj, prop_name, JS_DupValue(ctx, new_global));
        JS_FreeValue(ctx, global_obj);

        JS_FreeCString(ctx, prop_name);
        return new_global;
    }

    // Lazy define it (for example, defaulting to an empty object)
    JSValue new_global     = JS_UNDEFINED;
    global_vars[prop_name] = JS_DupValue(ctx, new_global);

    // Define on globalThis so it persists
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global_obj, prop_name, JS_DupValue(ctx, new_global));
    JS_FreeValue(ctx, global_obj);

    std::cout << "Lazy defined global: " << prop_name << std::endl;

    JS_FreeCString(ctx, prop_name);
    return new_global;
}

// Error handling helper function
static void js_dump_error(JSContext* ctx) {
    JSValue     exception = JS_GetException(ctx);
    const char* error_msg = JS_ToCString(ctx, exception);
    cerr << "Error: " << (error_msg ? error_msg : "unknown") << endl;
    if (error_msg) JS_FreeCString(ctx, error_msg);
    JS_FreeValue(ctx, exception);
}

// Custom console.log implementation
static JSValue js_console_log(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    for (int i = 0; i < argc; i++) {
        const char* str = JS_ToCString(ctx, argv[i]);
        if (str) {
            cout << str;
            JS_FreeCString(ctx, str);
        }
        if (i < argc - 1) cout << " ";
    }
    cout << endl;
    return JS_UNDEFINED;
}

void setup_js_env(JSContext* ctx) {
    // Get global object
    JSValue global_obj = JS_GetGlobalObject(ctx);

    // Create a QuickJS function that JS can call
    JS_SetPropertyStr(
        ctx, global_obj, "__lookup_global_from_cpp",
        JS_NewCFunction(ctx, js_lookup_global, "__lookup_global_from_cpp", 1)
    );

    // Inject JavaScript code to override globalThis with a Proxy
    const char* proxy_setup_code = R"(
        (function() {
            const nativeGlobalLookup = (name) => __lookup_global_from_cpp(name);

            globalThis = new Proxy(globalThis, {
                get(target, prop, receiver) {
                    if (!(prop in target)) {
                        return nativeGlobalLookup(prop);
                    }
                    return Reflect.get(target, prop, receiver);
                }
            });
        })();
    )";

    JSValue eval_result = JS_Eval(
        ctx, proxy_setup_code, strlen(proxy_setup_code), "<proxy-setup>", JS_EVAL_TYPE_GLOBAL
    );

    if (JS_IsException(eval_result)) {
        std::cerr << "Failed to setup Proxy for globalThis" << std::endl;
        js_dump_error(ctx);
    }

    JS_FreeValue(ctx, eval_result);
    JS_FreeValue(ctx, global_obj);
}

int main() {
    // Setup signal handling for CTRL+C
    signal(SIGINT, handle_signal);

    cout << "QuickJS REPL - Enter JavaScript code (double newline to execute, CTRL+C to exit)"
         << endl;
    cout << "> ";

    // Initialize QuickJS runtime with proper memory limits
    JSRuntime* runtime = JS_NewRuntime();
    if (!runtime) {
        cerr << "Failed to create JS runtime" << endl;
        return 1;
    }

    // Set memory limit (in bytes) to avoid overflow issues
    JS_SetMemoryLimit(runtime, 64 * 1024 * 1024);  // 64 MB

    // Set maximum stack size
    JS_SetMaxStackSize(runtime, 1024 * 1024);  // 1 MB

    // Create a JavaScript context
    JSContext* context = JS_NewContext(runtime);
    if (!context) {
        cerr << "Failed to create JS context" << endl;
        JS_FreeRuntime(runtime);
        return 1;
    }

    // Create global object manually instead of using JS_AddIntrinsicBaseObjects
    JSValue global = JS_GetGlobalObject(context);
    if (JS_IsException(global)) {
        cerr << "Failed to get global object" << endl;
        js_dump_error(context);
        JS_FreeContext(context);
        JS_FreeRuntime(runtime);
        return 1;
    }

    // Setup custom environment
    setup_js_env(context);

    // Add a console object with log method
    JSValue console = JS_NewObject(context);
    JS_SetPropertyStr(context, console, "log", JS_NewCFunction(context, js_console_log, "log", 1));
    JS_SetPropertyStr(context, global, "console", console);

    // Free the global object reference
    JS_FreeValue(context, global);

    // REPL loop
    string input_buffer;
    string current_line;
    bool   empty_line_detected = false;

    while (!ctrl_c_pressed) {
        getline(cin, current_line);

        // Check for CTRL+C during input
        if (ctrl_c_pressed) {
            cout << "\nExiting REPL..." << endl;
            break;
        }

        // Check for double newline (empty line)
        if (current_line.empty()) {
            if (empty_line_detected) {
                // Double newline detected, evaluate the code
                if (!input_buffer.empty()) {
                    cout << "Executing:" << endl;

                    std::string wrapped_code =
                        "(function() {\n"
                        "  try {\n"
                        "    return eval(`" +
                        input_buffer +
                        "`);\n"
                        "  } catch (e) {\n"
                        "    if (e instanceof ReferenceError && e.message.includes('is not "
                        "defined')) {\n"
                        "      const varName = e.message.split(' ')[0];\n"
                        "      globalThis[varName] = __lookup_global_from_cpp(varName);\n"
                        "      // Try again with the defined variable\n"
                        "      return eval(`" +
                        input_buffer +
                        "`);\n"
                        "    }\n"
                        "    throw e;\n"
                        "  }\n"
                        "})()";

                    JSValue result = JS_Eval(
                        context, wrapped_code.c_str(), wrapped_code.length(), "<repl>",
                        JS_EVAL_TYPE_GLOBAL
                    );

                    // Check for errors
                    if (JS_IsException(result)) {
                        js_dump_error(context);
                    } else if (!JS_IsUndefined(result)) {
                        // Print the result if it's not undefined
                        const char* result_str = JS_ToCString(context, result);
                        if (result_str) {
                            cout << "=> " << result_str << endl;
                            JS_FreeCString(context, result_str);
                        }
                    }

                    // Free the result value
                    JS_FreeValue(context, result);

                    // Reset input buffer after execution
                    input_buffer.clear();
                }

                empty_line_detected = false;
                cout << "> ";
            } else {
                empty_line_detected = true;
            }
        } else {
            // Add the current line to the input buffer
            if (!input_buffer.empty()) {
                input_buffer += "\n";
            }
            input_buffer += current_line;
            empty_line_detected = false;
        }
    }

    // Clean up
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);

    return 0;
}
