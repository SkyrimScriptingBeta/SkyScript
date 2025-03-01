#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

// Node.js includes
#include <node.h>
#include <v8.h>

// Define the version of Node.js API we're using
#define NODE_WANT_INTERNALS 1
#include <node_internals.h>

using namespace std;
using namespace v8;

std::unique_ptr<node::MultiIsolatePlatform> platform_;

// Store dynamically created globals
unordered_map<string, Global<Value>> global_vars;

// Flag to check if CTRL+C was pressed
volatile sig_atomic_t ctrl_c_pressed = 0;

// Signal handler for CTRL+C
void handle_signal(int signal) {
    if (signal == SIGINT) {
        ctrl_c_pressed = 1;
    }
}

// Environment setup
std::unique_ptr<node::CommonEnvironmentSetup> setup;

// C++ function exposed to JS - this is called when a global variable isn't found
void js_lookup_global(const FunctionCallbackInfo<Value>& args) {
    Isolate*       isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();

    cout << "C++ function called from JS" << endl;

    // Check that we have at least one argument and it's a string
    if (args.Length() < 1 || !args[0]->IsString()) {
        args.GetReturnValue().SetUndefined();
        return;
    }

    // Get the global name
    String::Utf8Value prop_name(isolate, args[0]);
    if (*prop_name == nullptr) {
        args.GetReturnValue().SetUndefined();
        return;
    }

    std::string name(*prop_name);
    cout << "Looking up global: " << name << endl;

    // Check if we already created this global
    auto it = global_vars.find(name);
    if (it != global_vars.end()) {
        args.GetReturnValue().Set(Local<Value>::New(isolate, it->second));
        return;
    }

    // If the prop name is "MyString" then lazily define a global string with the value "I am a
    // string!"
    if (name == "MyString") {
        Local<String> value = String::NewFromUtf8(isolate, "I am a string!").ToLocalChecked();

        // Store in our global vars map
        global_vars[name].Reset(isolate, value);

        // Define on global object so it persists
        Local<Object> global = context->Global();
        global->Set(context, args[0], value).Check();

        args.GetReturnValue().Set(value);
        return;
    }

    // Lazy define it (defaulting to undefined)
    args.GetReturnValue().SetUndefined();

    // Store in our global vars map
    Local<Value> undefined = Undefined(isolate);
    global_vars[name].Reset(isolate, undefined);

    // Define on global object so it persists
    Local<Object> global = context->Global();
    global->Set(context, args[0], undefined).Check();

    std::cout << "Lazy defined global: " << name << std::endl;
}

// Custom console.log implementation
void js_console_log(const FunctionCallbackInfo<Value>& args) {
    bool first = true;
    for (int i = 0; i < args.Length(); i++) {
        Isolate* isolate = args.GetIsolate();

        if (first) {
            first = false;
        } else {
            cout << " ";
        }

        String::Utf8Value str(isolate, args[i]);
        if (*str) {
            cout << *str;
        }
    }
    cout << endl;
}

void setup_js_env(Local<Context> context) {
    Isolate* isolate = context->GetIsolate();

    // Get global object
    Local<Object> global = context->Global();

    // Register the C++ function for global variable lookup
    Local<Function> lookup_func = Function::New(context, js_lookup_global).ToLocalChecked();
    global
        ->Set(
            context, String::NewFromUtf8(isolate, "__lookup_global_from_cpp").ToLocalChecked(),
            lookup_func
        )
        .Check();

    // Create console object with log method
    Local<Object> console = Object::New(isolate);
    console
        ->Set(
            context, String::NewFromUtf8(isolate, "log").ToLocalChecked(),
            Function::New(context, js_console_log).ToLocalChecked()
        )
        .Check();
    global->Set(context, String::NewFromUtf8(isolate, "console").ToLocalChecked(), console).Check();

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

            console.log("Global environment setup complete");
        })();
    )";

    // Compile and run the proxy setup code
    Local<String> source = String::NewFromUtf8(isolate, proxy_setup_code).ToLocalChecked();
    Local<Script> script = Script::Compile(context, source).ToLocalChecked();
    script->Run(context).ToLocalChecked();
}

// Error handling helper function
void print_error(Isolate* isolate, TryCatch& try_catch) {
    if (try_catch.HasCaught()) {
        Local<Value>      exception = try_catch.Exception();
        String::Utf8Value exception_str(isolate, exception);
        cerr << "Error: " << (*exception_str ? *exception_str : "unknown") << endl;

        // Print stack trace if available
        Local<Message> message = try_catch.Message();
        if (!message.IsEmpty()) {
            String::Utf8Value filename(isolate, message->GetScriptResourceName());
            int line = message->GetLineNumber(isolate->GetCurrentContext()).FromJust();
            cerr << "at " << (*filename ? *filename : "unknown") << ":" << line << endl;

            // Print line of source code
            String::Utf8Value sourceline(
                isolate, message->GetSourceLine(isolate->GetCurrentContext()).ToLocalChecked()
            );
            cerr << (*sourceline ? *sourceline : "") << endl;

            // Print wavy underline (^^^^^)
            int start = message->GetStartColumn(isolate->GetCurrentContext()).FromJust();
            int end   = message->GetEndColumn(isolate->GetCurrentContext()).FromJust();
            cerr << std::string(start, ' ') << std::string(end - start, '^') << endl;
        }

        // Print stack trace
        Local<Value> stack_trace;
        if (try_catch.StackTrace(isolate->GetCurrentContext()).ToLocal(&stack_trace) &&
            stack_trace->IsString() && Local<String>::Cast(stack_trace)->Length() > 0) {
            String::Utf8Value stack_trace_str(isolate, stack_trace);
            cerr << *stack_trace_str << endl;
        }
    }
}

int main() {
    // Setup signal handling for CTRL+C
    signal(SIGINT, handle_signal);

    cout << "Node.js REPL - Enter JavaScript code (double newline to execute, CTRL+C to exit)"
         << endl;
    cout << "> ";

    // Setup Node.js environment using the simplified CommonEnvironmentSetup
    // This handles platform setup, isolate creation and initialization
    std::vector<std::string> args = {"node", "--no-warnings"};
    std::vector<std::string> exec_args;
    std::vector<std::string> errors;

    node::InitializeOncePerProcess(
        args, {node::ProcessInitializationFlags::kNoInitializeV8,
               node::ProcessInitializationFlags::kNoInitializeNodeV8Platform}
    );

    platform_ = node::MultiIsolatePlatform::Create(4);
    v8::V8::InitializePlatform(platform_.get());
    v8::V8::Initialize();

    // We're leveraging Node.js's built-in setup utility to avoid manually configuring everything
    setup = node::CommonEnvironmentSetup::Create(platform_.get(), &errors, args, exec_args);

    if (!setup) {
        for (const auto& error : errors) {
            cerr << error << endl;
        }
        return 1;
    }

    Isolate* isolate = setup->isolate();

    {
        v8::Locker             locker(isolate);
        v8::Isolate::Scope     isolate_scope(isolate);
        v8::HandleScope        handle_scope(isolate);
        v8::Local<v8::Context> context = v8::Context::New(isolate);
        Context::Scope         context_scope(context);

        // Setup our custom JavaScript environment
        setup_js_env(context);

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

                        // Wrap code in a try/catch to handle missing globals
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

                        // Compile and run the code
                        TryCatch try_catch(isolate);

                        Local<String> source =
                            String::NewFromUtf8(isolate, wrapped_code.c_str()).ToLocalChecked();
                        Local<Script> script;

                        if (!Script::Compile(context, source).ToLocal(&script)) {
                            print_error(isolate, try_catch);
                        } else {
                            Local<Value> result;
                            if (!script->Run(context).ToLocal(&result)) {
                                print_error(isolate, try_catch);
                            } else if (!result->IsUndefined()) {
                                // Print the result
                                String::Utf8Value result_str(isolate, result);
                                cout << "=> " << (*result_str ? *result_str : "undefined") << endl;
                            }
                        }

                        // Process any Node.js events that might be pending
                        node::SpinEventLoop(setup->env()).ToChecked();

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
    }

    // Clean up Node.js environment
    if (setup) {
        // Properly clean up Node.js environment
        node::Stop(setup->env());
        setup.reset();
    }

    // Final cleanup
    node::TearDownOncePerProcess();

    return 0;
}
