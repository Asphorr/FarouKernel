// error_handler.hpp

#ifndef ERROR_HANDLER_HPP
#define ERROR_HANDLER_HPP

#include <concepts>
#include <functional>
#include <iostream>
#include <string>
#include <utility>

template <typename T>
concept ExceptionType = requires(T t) {
    typename T::ExceptionType;
};

template <typename T>
struct ErrorHandler : public std::enable_shared_from_this<ErrorHandler<T>> {
private:
    // Store the callback functions for each type of exception
    std::unordered_map<std::type_index, std::function<void(const std::exception&)>> _callbacks;

public:
    template <typename U>
    requires ExceptionType<U> && std::is_base_of_v<std::exception, U>
    void registerCallback(std::function<void(const U&)>&& func) {
        auto index = std::type_index(typeid(U));
        _callbacks[index] = std::move(func);
    }

    void handle(const std::exception& e) override {
        try {
            throw;
        } catch (const std::exception& ex) {
            auto iter = _callbacks.find(std::type_index(typeid(ex)));
            if (iter != _callbacks.end()) {
                (*iter)(e);
            } else {
                std::cerr << "Unhandled exception: " << e.what() << '\n';
            }
        }
    }
};

#endif /* ERROR_HANDLER_HPP */
