#pragma once
#include <ssh/async.h>
#include <ssh/handle.h>
#include <atomic>

namespace ssh {

class event;

class context {
public:
  context();

  context(context&& other) = delete;
  context& operator=(context&& other) = delete;

  context(const context& other) = delete;
  context& operator=(const context& other) = delete;

  ~context();

  explicit operator bool() const noexcept {
    return handle_.valid();
  }

  void run(std::size_t size = 1);

  void interrupt() noexcept;
  bool stop() noexcept;
  void reset() noexcept;

  ssh::async<void> schedule() noexcept;

  ssh::handle& handle() noexcept {
    return handle_;
  }

  const ssh::handle& handle() const noexcept {
    return handle_;
  }

private:
  std::atomic_uint32_t state_ = 0;
  ssh::handle handle_;
  ssh::handle events_;
};

const std::error_category& context_category() noexcept;

}  // namespace ssh
