# QuickJS Integration Example

This example demonstrates how to integrate QuickJS JavaScript engine into a C++ application.

## Troubleshooting

### Crash in JS_AddIntrinsicBaseObjects

The original code crashed during the call to `JS_AddIntrinsicBaseObjects()`. This could be due to several reasons:

1. **Incompatible QuickJS binary/library version** - Ensure that the QuickJS headers match the compiled library version
2. **Memory corruption** - Check if there are any memory issues in the application
3. **Initialization order** - The initialization order of QuickJS components matters

### Workaround

The current implementation avoids using the problematic helper functions and instead:

1. Manually gets the global object
2. Manually creates necessary JavaScript objects and functions
3. Uses direct JS API calls instead of higher-level helpers

## Building

Make sure to link against the QuickJS library:

```bash
g++ quickjs_example.cpp -o quickjs_example -lquickjs
```

Or with MSVC:

```
cl quickjs_example.cpp /link quickjs.lib
```

## Debugging QuickJS Issues

If you continue to face issues, consider:

1. Building QuickJS from source with debug symbols
2. Using a memory debugger like Valgrind or Address Sanitizer
3. Checking the exact version of QuickJS being used
