#include <quickjs.h>
#include <stdio.h>
#include <string.h>

#include <iostream>

using namespace std;

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

int main() {
    cout << "Hello, QuickJS world!" << endl;

    // Initialize QuickJS runtime with proper memory limits
    cout << "Initializing QuickJS runtime..." << endl;
    JSRuntime* runtime = JS_NewRuntime();
    if (!runtime) {
        cerr << "Failed to create JS runtime" << endl;
        return 1;
    }

    // Set memory limit (in bytes) to avoid overflow issues
    JS_SetMemoryLimit(runtime, 64 * 1024 * 1024);  // Increased to 64 MB

    // Set maximum stack size
    JS_SetMaxStackSize(runtime, 1024 * 1024);  // 1 MB

    cout << "QuickJS runtime initialized." << endl;

    // Create a JavaScript context
    cout << "Creating JavaScript context..." << endl;
    JSContext* context = JS_NewContext(runtime);
    if (!context) {
        cerr << "Failed to create JS context" << endl;
        JS_FreeRuntime(runtime);
        return 1;
    }
    cout << "JavaScript context created." << endl;

    // Try a minimal approach without using the helper functions that crash
    cout << "Setting up minimal environment..." << endl;

    // Create global object manually instead of using JS_AddIntrinsicBaseObjects
    JSValue global = JS_GetGlobalObject(context);
    if (JS_IsException(global)) {
        cerr << "Failed to get global object" << endl;
        js_dump_error(context);
        JS_FreeContext(context);
        JS_FreeRuntime(runtime);
        return 1;
    }

    // Add a console object with log method
    JSValue console = JS_NewObject(context);
    JS_SetPropertyStr(context, console, "log", JS_NewCFunction(context, js_console_log, "log", 1));
    JS_SetPropertyStr(context, global, "console", console);

    // Free the global object reference
    JS_FreeValue(context, global);

    cout << "Minimal environment setup complete." << endl;

    // Execute JavaScript code
    const char* js_code =
        "try { console.log('Hello from JavaScript!'); } catch(e) { console.log('Error: ' + e); }";
    cout << "Executing JavaScript code: " << js_code << endl;
    JSValue result = JS_Eval(context, js_code, strlen(js_code), "<input>", JS_EVAL_TYPE_GLOBAL);

    // Check for errors
    if (JS_IsException(result)) {
        cout << "JavaScript exception occurred." << endl;
        js_dump_error(context);
    } else {
        cout << "JavaScript code executed successfully." << endl;
        const char* result_str = JS_ToCString(context, result);
        if (result_str) {
            cout << "Result: " << result_str << endl;
            JS_FreeCString(context, result_str);
        }
    }

    // Free the result value
    JS_FreeValue(context, result);

    // Clean up
    cout << "Cleaning up..." << endl;
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    cout << "Clean up done." << endl;

    return 0;
}
