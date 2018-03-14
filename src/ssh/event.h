#pragma once
#include <ssh/async.h>
#include <cassert>

#if SSH_OS_WIN32
#include <windows.h>
#elif SSH_OS_LINUX
#include <sys/epoll.h>
#define SSH_EVENT_RECV EPOLLIN
#define SSH_EVENT_SEND EPOLLOUT
#elif SSH_OS_FREEBSD
#include <sys/event.h>
#define SSH_EVENT_RECV EVFILT_READ
#define SSH_EVENT_SEND EVFILT_WRITE
#endif

namespace ssh {

#if SSH_OS_WIN32
using event_base = OVERLAPPED;
#elif SSH_OS_LINUX
using event_base = epoll_event;
#elif SSH_OS_FREEBSD
using event_base = struct kevent;
#endif

class event final : public ssh::event_base {
public:
  using handle_type = std::experimental::coroutine_handle<>;

#if SSH_OS_WIN32
  event() noexcept : ssh::event_base({}) {
  }
#elif SSH_OS_LINUX
  event(int context, int fd, uint32_t events) noexcept : context_(context), fd_(fd) {
    const auto ev = static_cast<ssh::event_base*>(this);
    ev->data.ptr = ev;
    ev->events = events;
  }
#elif SSH_OS_FREEBSD
  event(int context, int fd, short filter, unsigned int fflags = 0) noexcept : context_(context) {
    const auto ev = static_cast<ssh::event_base*>(this);
    EV_SET(ev, static_cast<uintptr_t>(fd), filter, EV_ADD | EV_ONESHOT, fflags, 0, this);
  }
#endif

  event(event&& other) = delete;
  event& operator=(event&& other) = delete;

  event(const event& other) = delete;
  event& operator=(const event& other) = delete;

  constexpr bool await_ready() noexcept {
    return false;
  }

  void await_suspend(handle_type handle) noexcept {
    handle_ = handle;
#if SSH_OS_LINUX
    if (::epoll_ctl(context_, EPOLL_CTL_ADD, fd_, static_cast<ssh::event_base*>(this)) < 0) {
      error_ = errno;
      handle_.resume();
    }
#elif SSH_OS_FREEBSD
    if (::kevent(context_, static_cast<ssh::event_base*>(this), 1, nullptr, 0, nullptr) < 0) {
      error_ = errno;
      handle_.resume();
    }
#endif
  }

#if SSH_OS_WIN32
  constexpr DWORD await_resume() noexcept {
    return size_;
  }
#else
  constexpr int await_resume() noexcept {
    return error_;
  }
#endif

#if SSH_OS_WIN32
  void resume(DWORD size) noexcept {
    size_ = size;
    handle_.resume();
  }
#else
  void resume() noexcept {
#if SSH_OS_LINUX
    if (const auto ev = ::epoll_ctl(context_, EPOLL_CTL_DEL, fd_, static_cast<ssh::event_base*>(this)) < 0) {
      error_ = errno;
    }
#endif
    handle_.resume();
  }
#endif

  ssh::event_base* get() noexcept {
    return this;
  }

  const ssh::event_base* get() const noexcept {
    return this;
  }

private:
  handle_type handle_ = nullptr;
#if SSH_OS_WIN32
  DWORD size_ = 0;
#else
  int context_ = -1;
  int error_ = 0;
#endif
#if SSH_OS_LINUX
  int fd_ = -1;
#endif
};

}  // namespace ssh
