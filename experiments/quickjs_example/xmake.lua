add_requires("quickjs-ng")

target("quickjs_example")
    set_kind("binary")
    add_files("quickjs_example.cpp")
    add_packages("quickjs-ng")
