#include "NodejsEnvironment.h"

#define HAVE_INSPECTOR 1
#define NODE_WANT_INTERNALS 1

namespace Nodejs {

    NodejsEnvironment::NodejsEnvironment() { initialize(); }
    NodejsEnvironment::~NodejsEnvironment() { shutdown(); }

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

    void MyNamedPropertyGetter(
        v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info
    ) {
        LogText("[C++] Intercepting named property access...");
        v8::Isolate*    isolate = info.GetIsolate();
        v8::HandleScope handle_scope(isolate);

        v8::Local<v8::Context> context = isolate->GetCurrentContext();

        // Convert property name to a string for debugging
        v8::String::Utf8Value utf8(isolate, property);
        std::string           propName = *utf8 ? *utf8 : "<unknown>";
        LogText(fmt::format("[C++] Intercepted property access: '{}'", propName).c_str());

        // Get the object being accessed
        v8::Local<v8::Object> holder = info.This();

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

        //         bool hasReal = holder->HasRealNamedProperty(context, property).FromMaybe(false);
        // if (hasReal) {
        //   // ...
        // }

        if (hasReal) {
            // 2a) If it *does* exist, return that real property so we don't override it
            v8::MaybeLocal<v8::Value> realVal = holder->GetRealNamedProperty(context, property);
            if (!realVal.IsEmpty()) {
                info.GetReturnValue().Set(realVal.ToLocalChecked());
            }
            LogText("[C++] Real property found, returning it.");
            return;
        }

        // 2b) It's truly missing => define or return your "dummy" property
        // Example: create a function
        v8::Local<v8::FunctionTemplate> funcTemplate =
            v8::FunctionTemplate::New(isolate, MyInterceptedFunction);
        v8::Local<v8::Function> func = funcTemplate->GetFunction(context).ToLocalChecked();

        info.GetReturnValue().Set(func);
    }

    auto getterLambda = [](v8::Local<v8::Name>                        property,
                           const v8::PropertyCallbackInfo<v8::Value>& info) {
        MyNamedPropertyGetter(property, info);
    };

    Local<ObjectTemplate> CreateSandboxTemplate(Isolate* isolate) {
        Local<ObjectTemplate> global_templ = ObjectTemplate::New(isolate);

        v8::NamedPropertyHandlerConfiguration config(
            reinterpret_cast<v8::NamedPropertyGetterCallback>(+getterLambda
            ),                               // Use reinterpret_cast
            nullptr,                         // Setter
            nullptr,                         // Query
            nullptr,                         // Deleter
            nullptr,                         // Enumerator
            v8::Local<v8::Value>(),          // Data
            v8::PropertyHandlerFlags::kNone  // Flags
        );

        // Attach the interceptor to the global object template:
        global_templ->SetHandler(config);

        return global_templ;
    }

    v8::Global<v8::Context> sandboxContext_;

    void NodejsEnvironment::initialize() {
        LogText("Initializing Node.js environment...");

        // 1) Initialize Node’s process-level stuff (optional if you don’t actually need Node APIs).
        //    If you’re just using V8, you could skip some of this. But we’ll keep your existing
        //    code:
        std::vector<std::string> args = {"node"};
        std::vector<std::string> exec_args;
        args.push_back("--experimental-modules");
        args.push_back("--no-inspect");

        node::InitializeOncePerProcess(
            args, {node::ProcessInitializationFlags::kNoInitializeV8,
                   node::ProcessInitializationFlags::kNoInitializeNodeV8Platform}
        );

        // 2) Create a platform, initialize V8
        platform_ = node::MultiIsolatePlatform::Create(4);
        v8::V8::InitializePlatform(platform_.get());
        v8::V8::Initialize();

        // 3) (Optionally) Create CommonEnvironmentSetup,
        //    but we won't actually use setup_->context().
        std::vector<std::string> errors;
        setup_ = node::CommonEnvironmentSetup::Create(platform_.get(), &errors, args, exec_args);
        if (setup_) {
            LogText("Node.js environment initialized (but not used for scripts).");
        } else {
            for (auto& e : errors) {
                LogText(e.c_str());
            }
            return;
        }

        // 4) Create a new V8 isolate for your sandbox
        v8::Isolate* isolate = setup_->isolate();  // reusing the same isolate from Node or
        // If you prefer your own isolate, you could do:
        // v8::Isolate::CreateParams create_params;
        // create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
        // v8::Isolate* isolate = v8::Isolate::New(create_params);

        {
            v8::Locker         locker(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::HandleScope    handle_scope(isolate);

            // 5) Create *your* custom context with a named property interceptor
            Local<ObjectTemplate> global_templ = CreateSandboxTemplate(isolate);
            Local<Context> context = Context::New(isolate, /*extensions=*/nullptr, global_templ);

            // Store this context in a persistent member so you can reuse it in eval()
            sandboxContext_.Reset(isolate, context);

            Context::Scope context_scope(context);

            // 6) Define nativeLog on THIS context->Global()
            v8::Local<v8::Function> log_func =
                v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& info) {
                    for (int i = 0; i < info.Length(); ++i) {
                        v8::String::Utf8Value str(info.GetIsolate(), info[i]);
                        LogText(*str);
                    }
                }).ToLocalChecked();

            // context->Global()
            //     ->Set(
            //         context, v8::String::NewFromUtf8(isolate, "nativeLog").ToLocalChecked(),
            //         log_func
            //     )
            //     .Check();

            v8::Local<v8::Object> globalObj = context->Global();

            auto key     = v8::String::NewFromUtf8(isolate, "nativeLog").ToLocalChecked();
            bool success = globalObj
                               ->DefineOwnProperty(
                                   context, key, log_func,
                                   v8::PropertyAttribute::None  // or v8::ReadOnly, etc.
                               )
                               .FromMaybe(false);

            if (!success) {
                // handle error
            }

            LogText("Single sandbox context ready.");
        }
    }

    void NodejsEnvironment::shutdown() {
        LogText("Shutting down Node.js environment...");

        // Dispose the Node.js environment.
        if (setup_) {
            node::Stop(setup_->env());
            setup_.reset();
        }

        // Dispose V8 and the platform.
        v8::V8::Dispose();
        v8::V8::DisposePlatform();
        node::TearDownOncePerProcess();
    }

    // Define the ResolveCallback function.
    v8::MaybeLocal<v8::Module> NodejsEnvironment::ResolveCallback(
        v8::Local<v8::Context> context, v8::Local<v8::String> specifier,
        v8::Local<v8::FixedArray> import_assertions, v8::Local<v8::Module> referrer
    ) {
        // Module resolution logic goes here.
        // For simplicity, let's return an empty MaybeLocal to indicate module not found.
        return v8::MaybeLocal<v8::Module>();
    }

    // Set the ImportModuleDynamicallyCallback on the isolate.
    void SetImportModuleDynamicallyCallback(v8::Isolate* isolate) {
        isolate->SetHostImportModuleDynamicallyCallback(
            [](v8::Local<v8::Context> context, v8::Local<v8::Data> host_defined_options,
               v8::Local<v8::Value> resource_name, v8::Local<v8::String> specifier,
               v8::Local<v8::FixedArray> import_assertions) -> v8::MaybeLocal<v8::Promise> {
                // Dynamic import logic goes here.
                // For simplicity, return an empty MaybeLocal.
                return v8::MaybeLocal<v8::Promise>();
            }
        );
    }

    v8::MaybeLocal<v8::Value> NodejsEnvironment::eval(std::string_view code) {
        LogText("Evaluating JavaScript code...");

        v8::Isolate*       isolate = setup_->isolate();
        v8::Locker         locker(isolate);
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope    handle_scope(isolate);
        // v8::Context::Scope context_scope(setup_->context());

        v8::Local<v8::Context> localCtx = v8::Local<v8::Context>::New(isolate, sandboxContext_);
        v8::Context::Scope     context_scope(localCtx);

        //

        // 7) Create your “missing global” proxy inside this same context
        const char* proxyCode = R"(
globalThis.sandbox = new Proxy({}, {
  get(target, prop, receiver) {
    // If it exists on the real global, return that
    if (prop in globalThis) {
      return globalThis[prop];
    }
    // Otherwise define your “missing property” logic
    console.log("Missing property:", prop);
    target[prop] = function(...args) {
      console.log("Called missing property", prop, "with args:", args);
    };
    return target[prop];
  }
});

        )";

        // Compile/run
        v8::Local<v8::String> src = v8::String::NewFromUtf8(isolate, proxyCode).ToLocalChecked();
        v8::Local<v8::Script> script1 = v8::Script::Compile(localCtx, src).ToLocalChecked();
        script1->Run(localCtx).ToLocalChecked();

        //

        // Wrap the code in a function to simulate the Node.js module wrapper
        std::string wrapped_code = "(function(exports, require, module, __filename, __dirname) { " +
                                   std::string(code) + "\n})";

        // We'll build a wrapper:  with(myProxy){ ...userCode... }
        std::string with_wrapped_code = std::string("with(sandbox){ ") + wrapped_code + " }";

        // Convert the wrapped code to a V8 string
        v8::Local<v8::String> source;
        if (!v8::String::NewFromUtf8(isolate, with_wrapped_code.c_str(), v8::NewStringType::kNormal)
                 .ToLocal(&source)) {
            // Failed to create string.
            return v8::MaybeLocal<v8::Value>();
        }

        v8::TryCatch try_catch(isolate);

        // Compile the script
        v8::Local<v8::Script> script;
        if (!v8::Script::Compile(localCtx, source).ToLocal(&script)) {
            LogText("Compilation failed.");
            // Compilation failed.
            if (script.IsEmpty()) {
                auto                  exception = try_catch.Exception();
                v8::String::Utf8Value exception_str(isolate, exception);
                LogText(fmt::format("Exception: {}\n", *exception_str).c_str());
                return v8::MaybeLocal<v8::Value>();
            }
            return v8::MaybeLocal<v8::Value>();
        } else {
            LogText("Compilation succeeded.");
        }
        if (script.IsEmpty()) {
            auto                  exception = try_catch.Exception();
            v8::String::Utf8Value exception_str(isolate, exception);
            LogText(fmt::format("Exception: {}\n", *exception_str).c_str());
            return v8::MaybeLocal<v8::Value>();
        }

        // Run the script to get the function
        v8::Local<v8::Value> func_val;
        LogText(fmt::format("Running script: {}\n", code).c_str());
        if (!script->Run(localCtx).ToLocal(&func_val)) {
            // Running the script failed.
            LogText("Running script failed.");
            if (func_val.IsEmpty()) {
                auto                  exception = try_catch.Exception();
                v8::String::Utf8Value exception_str(isolate, exception);
                LogText(fmt::format("Exception: {}\n", *exception_str).c_str());
                return v8::MaybeLocal<v8::Value>();
            }
            return v8::MaybeLocal<v8::Value>();
        } else {
            LogText("Running script succeeded.");
        }
        if (func_val.IsEmpty()) {
            auto                  exception = try_catch.Exception();
            v8::String::Utf8Value exception_str(isolate, exception);
            LogText(fmt::format("Exception: {}\n", *exception_str).c_str());
            return v8::MaybeLocal<v8::Value>();
        }

        // Check if the result is a function
        if (!func_val->IsFunction()) {
            // Not a function.
            return v8::MaybeLocal<v8::Value>();
        }

        v8::Local<v8::Function> func = func_val.As<v8::Function>();

        // Prepare the arguments: exports, require, module, __filename, __dirname
        v8::Local<v8::Object> exports = v8::Object::New(isolate);
        v8::Local<v8::Object> module  = v8::Object::New(isolate);
        module
            ->Set(
                setup_->context(), v8::String::NewFromUtf8(isolate, "exports").ToLocalChecked(),
                exports
            )
            .Check();

        // Get the 'require' function from the global context
        v8::Local<v8::Value> require;
        if (!setup_->context()
                 ->Global()
                 ->Get(
                     setup_->context(), v8::String::NewFromUtf8(isolate, "require").ToLocalChecked()
                 )
                 .ToLocal(&require)) {
            // Failed to get 'require'
            return v8::MaybeLocal<v8::Value>();
        }

        // Prepare filename and dirname
        v8::Local<v8::String> filename =
            v8::String::NewFromUtf8(isolate, "embedded_script.js").ToLocalChecked();
        v8::Local<v8::String> dirname = v8::String::NewFromUtf8(isolate, ".").ToLocalChecked();

        v8::Local<v8::Value> args[] = {exports, require, module, filename, dirname};

        // Call the function
        auto callResult =
            func->Call(setup_->context(), v8::Undefined(isolate), 5, args).ToLocalChecked();

        // Spin the event loop to process any pending events.
        node::SpinEventLoop(setup_->env()).FromJust();

        // Return the module's exports
        return module->Get(
            setup_->context(), v8::String::NewFromUtf8(isolate, "exports").ToLocalChecked()
        );
    }

    v8::Isolate* NodejsEnvironment::getIsolate() { return setup_->isolate(); }

}  // namespace Nodejs

// // Expose 'require' to the global context
// v8::Local<v8::String> require_str =
//     v8::String::NewFromUtf8(isolate, "require").ToLocalChecked();

// // v8::Local<v8::Function> require_func = setup_->env()->builtin_module_require();
// v8::Local<v8::Function> require_func =
//     setup_->context()
//         ->Global()
//         ->Get(
//             setup_->context(),
//             v8::String::NewFromUtf8(isolate, "require").ToLocalChecked()
//         )
//         .ToLocalChecked()
//         .As<v8::Function>();
// setup_->context()->Global()->Set(setup_->context(), require_str,
// require_func).Check();