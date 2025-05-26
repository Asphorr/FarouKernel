#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>

using byte = unsigned char;

class Frame final {
public:
static std::shared_ptr<Frame> create(size_t length) noexcept;
~Frame() noexcept;

Frame(const Frame& other) noexcept;
Frame(Frame&& other) noexcept;
Frame& operator=(const Frame& other) noexcept;
Frame& operator=(Frame&& other) noexcept;

[[nodiscard]] byte* getData() const noexcept;
[[nodiscard]] size_t getLength() const noexcept;
[[nodiscard]] bool isKeyFrame() const noexcept;

void setIsKeyFrame(bool value) noexcept;
private:
Frame(byte* data, size_t length) noexcept;

std::unique_ptr<byte[]> m_data{new byte[length]{}};
size_t m_length{0};
bool m_isKeyFrame{false};
};

inline Frame::~Frame() noexcept = default;

inline Frame::Frame(const Frame& other) noexcept : m_data{other.m_data}, m_length{other.m_length} {}

inline Frame::Frame(Frame&& other) noexcept : m_data{std::move(other.m_data)}, m_length{other.m_length} {}

inline Frame& Frame::operator=(const Frame& other) noexcept {
m_data = other.m_data;
m_length = other.m_length;
return *this;
}

inline Frame& Frame::operator=(Frame&& other) noexcept {
m_data = std::move(other.m_data);
m_length = other.m_length;
return *this;
}

inline byte* Frame::getData() const noexcept {
return m_data.get();
}

inline size_t Frame::getLength() const noexcept {
return m_length;
}

inline bool Frame::isKeyFrame() const noexcept {
return m_isKeyFrame;
}

inline void Frame::setIsKeyFrame(bool value) noexcept {
m_isKeyFrame = value;
}

inline std::shared_ptr<Frame> Frame::create(size_t length) noexcept {
return std::make_shared<Frame>(new byte[length], length);
}

inline Frame::Frame(byte* data, size_t length) noexcept : m_data{data}, m_length{length} {}
