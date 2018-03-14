#include <ssh/context.h>
#include <ssh/event.h>
#include <ssh/exception.h>
#include <vector>
#include <cassert>
#include <cstdint>

#if SSH_OS_WIN32
#include <windows.h>
#include <winsock2.h>
#elif SSH_OS_LINUX
#include <sys/eventfd.h>
#endif

namespace ssh {
namespace {

constexpr std::uint32_t stop_requested_flag = 1;
constexpr std::uint32_t thread_count_increment = 2;

#if SSH_OS_WIN32
class library {
public:
  library() noexcept {
    WSADATA wsadata = {};
    if (const auto code = ::WSAStartup(MAKEWORD(2, 2), &wsadata)) {
      throw_error(code, "WSAStartup");
    }
    const auto major = LOBYTE(wsadata.wVersion);
    const auto minor = HIBYTE(wsadata.wVersion);
    if (major < 2 || (major == 2 && minor < 2)) {
      throw_error(std::errc::function_not_supported, "WSAStartup");
    }
  }

  std::error_code ec;
};
#endif

}  // namespace

context::context() {
#if SSH_OS_WIN32
  static library library;
  handle_.reset(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0));
  if (!handle_) {
    throw_error(::GetLastError(), "CreateIoCompletionPort");
  }
#elif SSH_OS_LINUX
  handle_.reset(::epoll_create1(0));
  if (!handle_) {
    throw_error(errno, "epoll_create1");
  }
  events_.reset(::eventfd(0, EFD_NONBLOCK));
  if (!events_) {
    throw_error(errno, "eventfd");
  }
  static epoll_event nev = {};
  nev.events = EPOLLONESHOT;
  if (::epoll_ctl(handle_.value(), EPOLL_CTL_ADD, events_.value(), &nev) < 0) {
    throw_error(errno, "epoll_ctl");
  }
#elif SSH_OS_FREEBSD
  handle_.reset(::kqueue());
  if (!handle_) {
    throw_error(errno, "kqueue");
  }
  struct kevent nev = {};
  EV_SET(&nev, 0, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
  if (::kevent(handle_.value(), &nev, 1, nullptr, 0, nullptr) < 0) {
    throw_error(errno, "kevent");
  }
#endif
}

context::~context() {
  [[maybe_unused]] const auto state = state_.fetch_or(stop_requested_flag, std::memory_order_release);
  [[maybe_unused]] const auto thread_count = state / thread_count_increment;
  assert(thread_count == 0);
}

void context::run(std::size_t size) {
#if SSH_OS_WIN32
  using data_type = OVERLAPPED_ENTRY;
  using size_type = ULONG;
#else
  using data_type = event_base;
  using size_type = int;
#endif
  auto error = 0;
  std::vector<data_type> events(size);
  const auto events_data = events.data();
  const auto events_size = static_cast<size_type>(events.size());
  state_.fetch_add(thread_count_increment, std::memory_order_relaxed);
  while ((state_.load(std::memory_order_acquire) & stop_requested_flag) == 0) {
#if SSH_OS_WIN32
    size_type count = 0;
    if (!::GetQueuedCompletionStatusEx(handle_.as<HANDLE>(), events_data, events_size, &count, INFINITE, FALSE)) {
      if (const auto code = ::GetLastError(); code != ERROR_ABANDONED_WAIT_0) {
        error = static_cast<int>(code);
        break;
      }
    }
#elif SSH_OS_LINUX
    size_type count = ::epoll_wait(handle_.value(), events_data, events_size, -1);
    if (count < 0 && errno != EINTR) {
      error = errno;
      break;
    }
#elif SSH_OS_FREEBSD
    size_type count = ::kevent(handle_.value(), nullptr, 0, events_data, events_size, nullptr);
    if (count < 0 && errno != EINTR) {
      error = errno;
      break;
    }
#endif
    for (size_type i = 0; i < count; i++) {
      auto& entry = events_data[i];
#if SSH_OS_WIN32
      if (entry.lpOverlapped) {
        static_cast<ssh::event*>(entry.lpOverlapped)->resume(entry.dwNumberOfBytesTransferred);
      }
#elif SSH_OS_LINUX
      if (entry.data.ptr) {
        reinterpret_cast<ssh::event*>(entry.data.ptr)->resume();
      }
#elif SSH_OS_FREEBSD
      if (entry.udata) {
        reinterpret_cast<ssh::event*>(entry.udata)->resume();
      }
#endif
    }
  }
  const auto state = state_.fetch_sub(thread_count_increment, std::memory_order_release);
  const auto stop_requested = state & stop_requested_flag;
  const auto thread_count = state / thread_count_increment;
  if (stop_requested && thread_count > 1) {
    interrupt();
  }
  throw_error(error, "run");
}

void context::interrupt() noexcept {
#if SSH_OS_WIN32
  ::PostQueuedCompletionStatus(handle_.as<HANDLE>(), 0, 0, nullptr);
#elif SSH_OS_LINUX
  static epoll_event nev = {};
  nev.events = EPOLLOUT | EPOLLONESHOT;
  ::epoll_ctl(handle_.value(), EPOLL_CTL_MOD, events_.value(), &nev);
#elif SSH_OS_FREEBSD
  static struct kevent nev = {};
  EV_SET(&nev, 0, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
  ::kevent(handle_.value(), &nev, 1, nullptr, 0, nullptr);
#endif
}

bool context::stop() noexcept {
  const auto state = state_.fetch_or(stop_requested_flag, std::memory_order_release);
  const auto stop_requested = state & stop_requested_flag;
  const auto thread_count = state / thread_count_increment;
  if (!stop_requested && thread_count > 0) {
    interrupt();
  }
  return thread_count == 0;
}

void context::reset() noexcept {
  [[maybe_unused]] const auto state = state_.fetch_and(~stop_requested_flag, std::memory_order_release);
  assert((state & stop_requested_flag) != 0);
}

ssh::async<void> context::schedule() noexcept {
#if SSH_OS_WIN32
  ssh::event ev;
  if (!::PostQueuedCompletionStatus(handle_.as<HANDLE>(), 0, 0, ev.get())) {
    co_return;
  }
  co_await ev;
#elif SSH_OS_LINUX
  const auto fd = ssh::handle(::eventfd(0, EFD_NONBLOCK));
  if (!fd) {
    co_return;
  }
  [[maybe_unused]] const auto code = co_await ssh::event(handle_.value(), fd.value(), SSH_EVENT_SEND);
#elif SSH_OS_FREEBSD
  [[maybe_unused]] const auto code = co_await ssh::event(handle_.value(), 0, EVFILT_USER, NOTE_TRIGGER);
#endif
  co_return;
}

class context_category_impl : public std::error_category {
public:
  using std::error_category::error_category;

  const char* name() const noexcept override {
    return "ssh::context";
  }

  std::string message(int ev) const override {
    return std::system_category().message(ev);
  }
};

context_category_impl g_context_category;

const std::error_category& context_category() noexcept {
  return g_context_category;
}

}  // namespace ssh
