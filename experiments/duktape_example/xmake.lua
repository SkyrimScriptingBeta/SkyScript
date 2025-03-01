add_requires("duktape")

target("duktape_example")
    set_kind("binary")
    add_files("duktape_example.cpp")
    add_packages("duktape")
