#ifndef CONCEPTS_H
#define CONCEPTS_H

template <typename T>
concept WritableType = requires(T t) {
   { t.write(std::declval<char[]>(), std::declval<size_t>())} -> std::same_as<void>;
};

template <typename T>
concept ReadableType = requires(T t) {
   { t.read(std::declval<char[]>(), std::declval<size_t>())} -> std::same_as<void>;
};

#endif
