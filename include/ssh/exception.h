#pragma once
#include <exception>
#include <stdexcept>
#include <system_error>

namespace ssh {

struct noerror_tag {};

constexpr inline noerror_tag noerror;

class domain_error : public std::domain_error {
public:
  using std::domain_error::domain_error;
};

class system_error : public std::system_error {
public:
  using std::system_error::system_error;
};

template <typename T>
inline void throw_error(T ev, const char* message) {
  if (const auto code = static_cast<int>(ev)) {
    throw system_error(std::error_code(code, std::system_category()), message);
  }
}

}  // namespace ice
