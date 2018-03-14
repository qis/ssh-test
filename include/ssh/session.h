#pragma once
#include <ssh/config.h>
#include <memory>

typedef struct ssh_session_struct* ssh_session;

namespace ssh {

enum class verbosity {
  nolog,
  warning,
  protocol,
  packet,
  functions,
};

class session {
public:
  explicit session(ssh::context& context);

  session(session&& other) noexcept = default;
  session& operator=(session&& other) noexcept = default;

  session(const session& other) = delete;
  session& operator=(const session& other) = delete;

  virtual ~session() = default;

  void set(ssh::verbosity verbosity);

  ssh::async<void> connect(const net::endpoint& endpoint);

  ssh_session handle() noexcept {
    return handle_.get();
  }

  const ssh_session handle() const noexcept {
    return handle_.get();
  }

private:
  std::unique_ptr<ssh_session_struct, void (*)(ssh_session)> handle_;
};

}  // namespace ssh
