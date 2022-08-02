#include <unetwork/tcp_server.h>

#include <userver/components/component.hpp>
#include <userver/engine/io.hpp>
#include <userver/logging/log.hpp>

#include <server/net/create_socket.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

using namespace userver;
namespace unetwork {

TCPServerConfig Parse(const userver::yaml_config::YamlConfig& value,
                      userver::formats::parse::To<TCPServerConfig>) {
  TCPServerConfig config;
  config.listener = value["listener"].As<userver::server::net::ListenerConfig>();
  config.clientsTaskProcessor = value["clients_task_processor"].As<std::string>();
  return config;
}

TCPServer::TCPServer(const TCPServerConfig& config,
                     const userver::components::ComponentContext& component_context) {
  listenerTask = userver::engine::CriticalAsyncNoSpan(
      component_context.GetTaskProcessor(config.listener.task_processor),
      [this](userver::engine::io::Socket&& listen_sock, userver::engine::TaskProcessor& cli_tp) {
        this->ServerRun(listen_sock, cli_tp);
      },
      userver::server::net::CreateSocket(config.listener),
      std::ref(component_context.GetTaskProcessor(config.clientsTaskProcessor)));
}

void TCPServer::ServerRun(userver::engine::io::Socket& listen_sock,
                          userver::engine::TaskProcessor& cli_tp) {
  while (!userver::engine::current_task::ShouldCancel()) {
    try {
      AcceptConnection(listen_sock, cli_tp);
    } catch (const userver::engine::io::IoCancelled&) {
      break;
    } catch (const std::exception& ex) {
      LOG_ERROR() << "can't accept connection: " << ex;
    }
  }
}

void TCPServer::Stop() {
  for (auto connWp : connections)
    if (auto connSp = connWp.lock()) connSp->Stop();
}

void TCPServer::AcceptConnection(engine::io::Socket& listen_sock, engine::TaskProcessor& cli_tp) {
  engine::io::Socket connSock = listen_sock.Accept({});
  connSock.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);
  LOG_DEBUG() << "New connection " << fmt::to_string(connSock.Getsockname());
  auto connection = makeConnection(std::move(connSock));
  onNewConnection(connection);
  connection->Start(cli_tp, connection);
}

void TCPServer::onNewConnection(std::shared_ptr<TCPConnection>& connection) {
  int fd = connection->Fd();
  if ((int)connections.size() < fd + 1) connections.resize(fd + 1);
  connections[fd] = connection;
}

}  // namespace unetwork
