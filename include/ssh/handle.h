#pragma once
#include <ssh/config.h>
#include <type_traits>
#include <utility>
#include <cstdint>

namespace ssh {

class handle {
public:
#if SSH_OS_WIN32
  using value_type = std::intptr_t;
  constexpr static value_type invalid_value = 0;
#else
  using value_type = int;
  constexpr static value_type invalid_value = -1;
#endif
  using close_type = void (*)(handle& handle) noexcept;

  handle() noexcept = default;

  template <typename T>
  constexpr explicit handle(T value, close_type close = default_close) noexcept : close_(close) {
    if constexpr (std::is_pointer_v<T>) {
      value_ = reinterpret_cast<value_type>(value);
    } else {
      value_ = static_cast<value_type>(value);
    }
  }

  handle(handle&& other) noexcept : close_(other.close_), value_(other.release()) {
  }

  handle& operator=(handle&& other) noexcept {
    if (this != std::addressof(other)) {
      reset(other.release());
    }
    return *this;
  }

  handle(const handle& other) = delete;
  handle& operator=(const handle& other) = delete;

  ~handle() {
    if (value_ != invalid_value && close_) {
      close_(*this);
    }
  }

  constexpr bool operator==(const handle& other) const noexcept {
    return value_ == other.value_;
  }

  constexpr bool operator!=(const handle& other) const noexcept {
    return value_ != other.value_;
  }

  constexpr bool valid() const noexcept {
    return value_ != invalid_value;
  }

  constexpr explicit operator bool() const noexcept {
    return valid();
  }

  template <typename T>
  constexpr T as() const noexcept {
    if constexpr (std::is_pointer_v<T>) {
      return reinterpret_cast<T>(value_);
    } else {
      return static_cast<T>(value_);
    }
  }

  constexpr value_type value() const noexcept {
    return value_;
  }

  void close() noexcept {
    if (value_ != invalid_value) {
      if (close_) {
        close_(*this);
      }
      value_ = invalid_value;
    }
  }

  template <typename T>
  void reset(T value) noexcept {
    if (value_ != invalid_value && close_) {
      close_(*this);
    }
    if constexpr (std::is_pointer_v<T>) {
      value_ = reinterpret_cast<value_type>(value);
    } else {
      value_ = static_cast<value_type>(value);
    }
  }

  void reset() noexcept {
    reset(invalid_value);
  }

  value_type release() noexcept {
    return std::exchange(value_, invalid_value);
  }

  static void default_close(ssh::handle& handle) noexcept;

protected:
  close_type close_ = default_close;
  value_type value_ = invalid_value;
};

}  // namespace ssh
