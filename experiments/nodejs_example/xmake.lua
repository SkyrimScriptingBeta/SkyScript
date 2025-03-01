local vs_runtime = is_mode("debug") and "MTd" or "MT"
set_runtimes(vs_runtime)

target("nodejs_example")
    set_kind("binary")
    add_files("*.cpp")
    link_to_nodejs("../../modules/node")
