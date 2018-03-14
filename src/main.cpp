#include <ice/context.h>
#include <ice/ssh/session.h>
#include <array>
#include <iostream>

// clang-format off

#if 1

ssh::task start(ssh::context& context) {
  ssh::ssh::session session(context);
  session.set(ssh::ssh::verbosity::functions);
  context.stop();
  co_return;
}

// clang-format on

#else

ssh::task handle(ssh::net::tcp::socket client) {
  std::cout << "server: connection opened: " << client.handle().value() << std::endl;
  std::array<char, 1024> buffer;
  client.set(ssh::net::option::nodelay(true));
  while (true) {
    if (const auto ec = co_await client.await_send()) {
      std::cerr << "await send: " << ssh::format(ec) << std::endl;
      break;
    }
    std::cout << "send possible" << std::endl;
    if (const auto ec = co_await client.await_recv()) {
      std::cerr << "await recv: " << ssh::format(ec) << std::endl;
      break;
    }
    std::cout << "recv possible" << std::endl;
    const auto data = co_await client.recv(buffer);
    if (data.empty()) {
      break;
    }
    if (const auto sent = co_await client.send(data); sent != data.size()) {
      std::cerr << sent << " != " << data.size() << std::endl;
      break;
    }
  }
  std::cout << "server: connection closed: " << client.handle().value() << std::endl;
}

ssh::task start(ssh::context& context) {
  ssh::net::endpoint endpoint;
  endpoint.create("127.0.0.1", 9999);
  ssh::net::tcp::socket server(context);
  server.create(endpoint.family());
  server.set(ssh::net::option::reuseaddr(true));
  server.bind(endpoint);
  server.listen();
  for co_await(auto&& client : server.accept()) {
    handle(std::move(client));
  }
  context.stop();
  co_return;
}

#endif

int main(int argc, char* argv[]) {
  std::error_code ec;
  ssh::context context;
  context.create(ec);
  if (ec) {
    std::cerr << ssh::format(ec) << std::endl;
    return ec.value();
  }
  start(context);
  context.run(ec);
  if (ec) {
    std::cerr << ssh::format(ec) << std::endl;
    return ec.value();
  }
}
