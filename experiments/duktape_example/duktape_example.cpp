#include <signal.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <string>
#include <unordered_map>

#include "duktape.h"

using namespace std;

// Store dynamically created globals
unordered_map<string, duk_idx_t> global_vars;

// Flag to check if CTRL+C was pressed
volatile sig_atomic_t ctrl_c_pressed = 0;

// Signal handler for CTRL+C
void handle_signal(int signal) {
    if (signal == SIGINT) {
        ctrl_c_pressed = 1;
    }
}

// Helper function to push a string to the stack and return its index
duk_idx_t push_string_to_stash(duk_context* ctx, const char* str) {
    // Use the stash to store values persistently
    duk_push_global_stash(ctx);

    // Generate a unique key for the stash
    static int stash_counter = 0;
    char       key[32];
    sprintf(key, "_stashed_%d", stash_counter++);

    // Store the value in the stash
    duk_push_string(ctx, str);
    duk_put_prop_string(ctx, -2, key);

    // Pop the stash
    duk_pop(ctx);

    return stash_counter - 1;
}

// C++ function exposed to JS
duk_ret_t js_lookup_global(duk_context* ctx) {
    cout << "C++ function called from JS" << endl;

    // Check that we have at least one argument and it's a string
    if (!duk_is_string(ctx, 0)) {
        duk_push_undefined(ctx);
        return 1;
    }

    // Get the global name
    const char* prop_name = duk_get_string(ctx, 0);

    if (!prop_name) {
        duk_push_undefined(ctx);
        return 1;
    } else {
        cout << "Looking up global: " << prop_name << endl;
    }

    // Check if we already created this global
    auto it = global_vars.find(prop_name);
    if (it != global_vars.end()) {
        // Push the global onto the stack
        duk_push_global_stash(ctx);
        char key[32];
        sprintf(key, "_stashed_%d", it->second);
        duk_get_prop_string(ctx, -1, key);
        duk_remove(ctx, -2);  // Remove stash, leaving only the value
        return 1;
    }

    // If the prop name is "MyString" then lazily define a global string with the value "I am a
    // string!"
    if (strcmp(prop_name, "MyString") == 0) {
        duk_push_string(ctx, "I am a string!");

        // Store in our global vars map
        duk_idx_t idx                       = push_string_to_stash(ctx, "I am a string!");
        global_vars[std::string(prop_name)] = idx;

        // Define on global object so it persists
        duk_push_global_object(ctx);
        duk_push_string(ctx, "I am a string!");
        duk_put_prop_string(ctx, -2, prop_name);
        duk_pop(ctx);

        return 1;
    }

    // Lazy define it (defaulting to undefined)
    duk_push_undefined(ctx);

    // Store in our global vars map and define on global object
    duk_idx_t idx                       = push_string_to_stash(ctx, "undefined");
    global_vars[std::string(prop_name)] = idx;

    // Define on global object so it persists
    duk_push_global_object(ctx);
    duk_push_undefined(ctx);
    duk_put_prop_string(ctx, -2, prop_name);
    duk_pop(ctx);

    std::cout << "Lazy defined global: " << prop_name << std::endl;

    return 1;
}

// Custom console.log implementation
duk_ret_t js_console_log(duk_context* ctx) {
    int nargs = duk_get_top(ctx);

    for (int i = 0; i < nargs; i++) {
        const char* str = duk_safe_to_string(ctx, i);
        if (str) {
            cout << str;
        }
        if (i < nargs - 1) cout << " ";
    }
    cout << endl;

    return 0;  // no return value
}

// Error handler for duktape
void print_error(duk_context* ctx) {
    if (duk_is_error(ctx, -1)) {
        // Accessing .stack might cause an error
        duk_get_prop_string(ctx, -1, "stack");
        cerr << "Error: " << duk_safe_to_string(ctx, -1) << endl;
        duk_pop(ctx);
    } else {
        cerr << "Error: " << duk_safe_to_string(ctx, -1) << endl;
    }
    duk_pop(ctx);
}

void setup_js_env(duk_context* ctx) {
    // Register the C++ function that JS can call
    duk_push_global_object(ctx);
    duk_push_c_function(ctx, js_lookup_global, 1);
    duk_put_prop_string(ctx, -2, "__lookup_global_from_cpp");
    duk_pop(ctx);

    // Setup console.log
    duk_push_global_object(ctx);
    duk_push_object(ctx);
    duk_push_c_function(ctx, js_console_log, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "log");
    duk_put_prop_string(ctx, -2, "console");
    duk_pop(ctx);

    // Inject JavaScript code to override global object with a Proxy
    const char* proxy_setup_code = R"(
        (function() {
            var nativeGlobalLookup = function(name) {
                return __lookup_global_from_cpp(name);
            };

            // We need to be more careful with the global object in Duktape
            // Instead of trying to modify existing properties, we'll intercept lookups
            // using our own function

            // Store the original global object's getter
            var originalGetProp = Object.getOwnPropertyDescriptor(Object.prototype, '__lookupGetter__').value;
            var originalGetDesc = Object.getOwnPropertyDescriptor;

            // Add our lookup function to the global object
            this.lookupGlobal = function(prop) {
                if (!(prop in this)) {
                    return nativeGlobalLookup(prop);
                }
                return this[prop];
            };

            // Define a few helper methods to work with our global interception
            this.hasGlobal = function(prop) {
                return prop in this || typeof nativeGlobalLookup(prop) !== 'undefined';
            };

            // Trying to use a proper Proxy isn't reliable in Duktape, so we'll use this approach instead
            this.__getGlobalVar = function(name) {
                if (!(name in this)) {
                    // Lazily define the global via our C++ function
                    return nativeGlobalLookup(name);
                }
                return this[name];
            };

            console.log("Global environment setup complete");
        })();
    )";

    if (duk_peval_string(ctx, proxy_setup_code) != 0) {
        std::cerr << "Failed to setup global interception" << std::endl;
        print_error(ctx);
    }
    duk_pop(ctx);  // pop result

    // Now modify our error wrapper to use the new approach
    const char* error_wrapper_setup = R"(
        (function() {
            // We'll save the original eval
            var originalEval = eval;

            // Redefine global eval to catch ReferenceErrors
            this.eval = function(code) {
                try {
                    return originalEval(code);
                } catch (e) {
                    if (e instanceof ReferenceError && e.message.indexOf('not defined') !== -1) {
                        var varName = e.message.split(' ')[0];
                        // Try to load the variable using our C++ function
                        this[varName] = __lookup_global_from_cpp(varName);
                        // Try again
                        return originalEval(code);
                    }
                    throw e;
                }
            };
        })();
    )";

    if (duk_peval_string(ctx, error_wrapper_setup) != 0) {
        std::cerr << "Failed to setup error wrapper" << std::endl;
        print_error(ctx);
    }
    duk_pop(ctx);  // pop result
}

int main() {
    // Setup signal handling for CTRL+C
    signal(SIGINT, handle_signal);

    cout << "Duktape REPL - Enter JavaScript code (double newline to execute, CTRL+C to exit)"
         << endl;
    cout << "> ";

    // Create Duktape context
    duk_context* ctx = duk_create_heap_default();
    if (!ctx) {
        cerr << "Failed to create Duktape context" << endl;
        return 1;
    }

    // Setup custom environment
    setup_js_env(ctx);

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

                    // No need for complex wrapping since we've modified the global eval
                    // Just evaluate directly
                    if (duk_peval_string(ctx, input_buffer.c_str()) != 0) {
                        // Handle error
                        print_error(ctx);
                    } else if (!duk_is_undefined(ctx, -1)) {
                        // Print the result
                        cout << "=> " << duk_safe_to_string(ctx, -1) << endl;
                    }

                    // Pop the result
                    duk_pop(ctx);

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
    duk_destroy_heap(ctx);

    return 0;
}
