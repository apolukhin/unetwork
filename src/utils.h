#pragma once
#include <span>
#include <userver/engine/io/socket.hpp>
#include <userver/yaml_config/yaml_config.hpp>
#include <unetwork/http_server.h>

namespace unetwork {

inline void SendExactly(userver::engine::io::Socket& socket, std::span<const std::byte> data,
                        userver::engine::Deadline deadline) {
  if (socket.SendAll(data.data(), data.size(), deadline) != data.size())
    throw(userver::engine::io::IoException() << "Socket closed during transfer ");
}

inline void RecvExactly(userver::engine::io::Socket& socket, std::span<std::byte> buffer,
                        userver::engine::Deadline deadline) {
  if (socket.RecvAll(buffer.data(), buffer.size(), deadline) != buffer.size())
    throw(userver::engine::io::IoException() << "Socket closed during transfer ");
}

inline bool TestHeaderVal(const http::Headers& headers, std::string_view key,
                          std::string_view val) {
  auto it = headers.find(key);
  if (it == headers.end()) return false;
  return it->second == val;
}

inline const std::string& GetOptKey(const http::Headers& headers, std::string_view key,
                                    const std::string& def) {
  auto it = headers.find(key);
  if (it != headers.end()) return it->second;
  return def;
}

};  // namespace unetwork
