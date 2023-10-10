#pragma once

#include <exception>

class ErrorHandler {
public:
    virtual ~ErrorHandler() = default;
    virtual void handle(const std::exception& e) = 0;
};
