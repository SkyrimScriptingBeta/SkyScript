-- https://github.com/nodejs/node/issues/27189
-- https://github.com/nodejs/node/issues/28218

local node_folder = "../../../modules/node"

function link_to_nodejs()
    add_includedirs(
        node_folder .. "/src",
        node_folder .. "/deps/uv/include",
        node_folder .. "/deps/v8/include",
        node_folder .. "/deps/simdjson"
    )
    add_links("ada", "brotli", "cares", "histogram", "icudata", "icui18n", "icutools", "icuucx", "libnode", "libuv", "llhttp", "nbytes", "ncrypto", "nghttp2", "nghttp3", "ngtcp2", "openssl", "simdjson", "simdutf", "sqlite", "torque_base", "uvwasi", "v8_abseil", "v8_base_without_compiler", "v8_compiler", "v8_init", "v8_initializers_slow", "v8_initializers", "v8_libbase", "v8_libplatform", "v8_snapshot", "v8_turboshaft", "v8_zlib", "zlib_adler32_simd", "zlib_inflate_chunk_simd", "zlib");
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

target("Some Keyboard Binds")
    add_files("*.cpp")

    add_packages("skyrim-commonlib")
    add_rules("@skyrim-commonlib/plugin", {
        mods_folder = os.getenv("SKYSCRIPT_MODS_FOLDER")
    })
    compile_papyrus_scripts()
    copy_files_into_mod_folder({ "Scripts" })

    link_to_nodejs()