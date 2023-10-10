#pragma once

#include <memory>

struct ConsoleInterface {
    virtual ~ConsoleInterface() = default;
    virtual void run() = 0;
};
