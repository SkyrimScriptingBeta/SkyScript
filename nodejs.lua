-- https://github.com/nodejs/node/issues/27189
-- https://github.com/nodejs/node/issues/28218
-- https://github.com/nodejs/node/blob/main/BUILDING.md
-- https://github.com/nodejs/node/blob/main/BUILDING.md#windows

function link_to_nodejs(node_folder)
    add_includedirs(
        node_folder .. "/src",
        node_folder .. "/deps/uv/include",
        node_folder .. "/deps/v8/include",
        node_folder .. "/deps/simdjson"
    )
    add_links("ada", "brotli", "crdtp", "cares", "histogram", "icudata", "icui18n", "icutools", "icuucx", "libnode", "libuv", "llhttp", "nbytes", "ncrypto", "nghttp2", "openssl", "simdjson", "simdutf", "sqlite", "torque_base", "uvwasi", "v8_abseil", "v8_base_without_compiler", "v8_compiler", "v8_init", "v8_initializers_slow", "v8_initializers", "v8_libbase", "v8_libplatform", "v8_snapshot", "v8_turboshaft", "v8_zlib", "zlib_adler32_simd", "zlib_inflate_chunk_simd", "zlib", "zstd");
    if is_mode("debug") then
        add_linkdirs(node_folder .. "/out/Debug/lib")
    else
        add_linkdirs(node_folder .. "/out/Release/lib")
    end
    if is_plat("windows") then
        add_cxflags("/Zc:__cplusplus")
        add_links("winmm", "dbghelp", "shlwapi")
        add_links("user32", "userenv", "ole32", "shell32", "crypt32", "psapi")
    end
end
