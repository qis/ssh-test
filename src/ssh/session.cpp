#include <ice/ssh/session.h>
#include <libssh/libssh.h>

namespace ssh::ssh {

session::session(ssh::context& context) : handle_(::ssh_new(), ::ssh_free), socket_(context) {
  if (!handle_) {
    throw ssh::domain_error("Could not create ssh session");
  }
  ssh_set_blocking(handle(), 1);
  //ssh_options_set(handle(), SSH_OPTIONS_HOST, "localhost");
  //ssh_options_set(handle(), SSH_OPTIONS_PORT, &port);
  throw ssh::domain_error(std::to_string(::ssh_get_fd(handle())));
}

void session::set(ssh::verbosity verbosity) {
  int value = SSH_LOG_NOLOG;
  switch (verbosity) {
  case ssh::verbosity::nolog: value = SSH_LOG_NOLOG; break;
  case ssh::verbosity::warning: value = SSH_LOG_WARNING; break;
  case ssh::verbosity::protocol: value = SSH_LOG_PROTOCOL; break;
  case ssh::verbosity::packet: value = SSH_LOG_PACKET; break;
  case ssh::verbosity::functions: value = SSH_LOG_FUNCTIONS; break;
  }
  if (ssh_options_set(handle(), SSH_OPTIONS_LOG_VERBOSITY, &value)) {
    throw ssh::domain_error(ssh_get_error(handle()));
  }
}

ssh::async<void> session::connect(const net::endpoint& endpoint) {
  endpoint.address().c_str();
  ssh_options_set(handle(), SSH_OPTIONS_HOST, "localhost");
  //ssh_options_set(handle(), SSH_OPTIONS_PORT, &port);
  co_return;
}

}  // namespace ssh::ssh
