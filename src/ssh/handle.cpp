#include <ice/handle.h>
#include <cerrno>

#if SSH_OS_WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace ice {

void handle::default_close(ssh::handle& handle) noexcept {
#if SSH_OS_WIN32
  ::CloseHandle(handle.as<HANDLE>());
#else
  while (::close(handle.value()) < 0) {
    if (errno != EINTR) {
      break;
    }
  }
#endif
  handle.release();
}

}  // namespace ice
