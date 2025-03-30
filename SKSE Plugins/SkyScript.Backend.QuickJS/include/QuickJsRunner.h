#pragma once

#include <string>
#include <string_view>

class QuickJsRunner {
public:
    QuickJsRunner();
    ~QuickJsRunner();

    std::string ExecuteAndReturnStringifiedResult(std::string_view script);
};
