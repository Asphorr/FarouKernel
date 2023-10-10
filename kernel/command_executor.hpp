#include <iostream>
#include <memory>
#include <utility>
#include <type_traits>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <functional>

template <typename T>
using unique_ptr = std::unique_ptr<T>;

template <typename T>
using shared_ptr = std::shared_ptr<T>;

template <typename T>
using weak_ptr = std::weak_ptr<T>;

template <typename T>
using enable_if = typename std::enable_if<T>::type;

template <typename T>
using disable_if = typename std::disable_if<T>::type;

template <typename T>
using remove_reference = typename std::remove_reference<T>::type;

template <typename T>
using decay = typename std::decay<T>::type;

template <typename... Args>
struct overload : public Args... {};

template <typename... Args>
overload(Args...) -> overload<Args...>;

template <typename T>
concept bool IsUniquePtr = requires(T t) {
    { t.get() };
    { *t };
    { t->operator*() };
};

template <typename T>
concept bool IsSharedPtr = requires(T t) {
    { t.use_count() };
    { *t };
    { t->operator*() };
};

template <typename T>
concept bool IsWeakPtr = requires(T t) {
    { t.lock().get() };
    { *t };
    { t->operator*() };
};

template <typename T>
concept bool IsSmartPointer = IsUniquePtr<T> || IsSharedPtr<T> || IsWeakPtr<T>;

template <typename T>
concept bool IsIterator = requires(T t) {
    { ++t };
    { *t };
    { t->operator*() };
};

template <typename T>
concept bool IsRange = requires(T t) {
    { begin(t) };
    { end(t) };
    { distance(begin(t), end(t)) };
};

template <typename T>
concept bool IsContainer = requires(T t) {
    { size(t) };
    { empty(t) };
    { front(t) };
    { back(t) };
    { push_back(t) };
    { pop_back(t) };
    { emplace_back(t) };
    { erase(t) };
    { swap(t) };
};

template <typename T>
concept bool IsMap = requires(T t) {
    { t.size() };
    { t.empty() };
    { t.clear() };
    { t.insert() };
    { t.erase() };
    { t.at() };
    { t.find() };
    { t.lower_bound() };
    { t.upper_bound() };
    { t.equal_range() };
};

template <typename T>
concept bool IsFunction = requires(T t) {
    { t() };
};

template <typename T>
concept bool IsCallable = IsFunction<T> || IsLambda<T>;

template <typename T>
concept bool IsLambda = requires(T t) {
    { t() };
};

template <typename T>
concept bool IsTuple = requires(T t) {
    { tuple_size<T>() };
    { get<0>(t) };
    { get<1>(t) };
    { get<2>(t) };
    { get<3>(t) };
    { get<4>(t) };
    { get<5>(t) };
    { get<6>(t) };
    { get<7>(t) };
    { get<8>(t) };
    { get<9>(t) };
    { get<10>(t) };
    { get<11>(t) };
    { get<12>(t) };
    { get<13>(t) };
    { get<14>(t) };
    { get<15>(t) };
    { get<16>(t) };
    { get<17>(t) };
    { get<18>(t) };
    { get<19>(t) };
    { get<20>(t) };
};

template <typename T>
concept bool IsPair = requires(T t) {
    { first(t) };
    { second(t) };
};

template <typename T>
concept bool IsString = requires(T t) {
    { string(t) };
};

template <typename T>
concept bool IsNumber = requires(T t) {
    { number(t) };
};

template <typename T>
concept bool IsBool = requires(T t) {
    { boolean(t) };
};

template <typename T>
concept bool IsChar = requires(T t) {
    { char(t) };
};

template <typename T>
concept bool IsVoid = requires(T t) {
    { void(t) };
};

template <typename T>
concept bool IsNull = requires(T t) {
    { null(t) };
};

template <typename T>
concept bool IsUndefined = requires(T t) {
    { undefined(t) };
};

template <typename T>
concept bool IsObject = requires(T t) {
    { object(t) };
};

template <typename T>
concept bool IsArray = requires(T t) {
    { array(t) };
};

template <typename T>
concept bool IsEnum = requires(T t) {
    { enum(t) };
};

template <typename T>
concept bool IsUnion = requires(T t) {
    { union(t) };
};

template <typename T>
concept bool IsStruct = requires(T t) {
    { struct(t) };
};

template <typename T>
concept bool IsClass = requires(T t) {
    { class(t) };
};

template <typename T>
concept bool IsInterface = requires(T t) {
    { interface(t) };
};

template <typename T>
concept bool IsAbstract = requires(T t) {
    { abstract(t) };
};

template <typename T>
concept bool IsVirtual = requires(T t) {
    { virtual(t) };
};

template <typename T>
concept bool IsOverride = requires(T t) {
    { override(t) };
};

template <typename T>
concept bool IsFinal = requires(T t) {
    { final(t) };
};

template <typename T>
concept bool IsStatic = requires(T t) {
    { static(t) };
};

template <typename T>
concept bool IsConstexpr = requires(T t) {
    { constexpr(t) };
};

template <typename T>
concept bool IsNoexcept = requires(T t) {
    { noexcept(t) };
};

template <typename T>
concept bool IsDefaultConstructible = requires(T t) {
    { new T{} };
};

template <typename T>
concept bool IsCopyConstructible = requires(T t) {
    { new T{} };
};
