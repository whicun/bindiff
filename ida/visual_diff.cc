// Copyright 2011-2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "third_party/zynamics/bindiff/ida/visual_diff.h"

#ifdef _WIN32
#define _WIN32_WINNT 0x0501
#include <windows.h>  // NOLINT
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>  // NOLINT
#include <thread>     // NOLINT

#include "third_party/absl/cleanup/cleanup.h"
#include "third_party/absl/functional/function_ref.h"
#include "third_party/absl/status/status.h"
#include "third_party/absl/strings/str_cat.h"
#include "third_party/absl/strings/string_view.h"
#include "third_party/absl/time/clock.h"
#include "third_party/zynamics/bindiff/config.h"
#include "third_party/zynamics/bindiff/differ.h"
#include "third_party/zynamics/bindiff/flow_graph.h"
#include "third_party/zynamics/bindiff/match/context.h"
#include "third_party/zynamics/bindiff/start_ui.h"
#include "third_party/zynamics/binexport/util/filesystem.h"
#include "third_party/zynamics/binexport/util/status_macros.h"

namespace security::bindiff {

#ifdef _WIN32
// Use the original BSD names for this
int(__stdcall* close)(SOCKET) = closesocket;
#endif

absl::Status DoSendGuiMessageTCP(absl::string_view server, uint16_t port,
                                 absl::string_view arguments) {
#ifdef _WIN32
  static int winsock_status = []() -> int {
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(2, 2), &wsa_data);
  }();
  if (winsock_status != 0) {
    return absl::UnknownError(
        absl::StrCat("WSAStartup failed with error: ", winsock_status));
  }
#endif

  uint32_t packet_size = arguments.size();
  std::string packet(reinterpret_cast<const uint8_t*>(&packet_size),
                     reinterpret_cast<const uint8_t*>(&packet_size) + 4);
  absl::StrAppend(&packet, arguments);

  struct addrinfo hints = {0};
  hints.ai_family = AF_UNSPEC;  // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV;
  hints.ai_protocol = IPPROTO_TCP;
  struct addrinfo* address_info = nullptr;
  auto err = getaddrinfo(std::string(server).c_str(),
                         absl::StrCat(port).c_str(), &hints, &address_info);
  if (err != 0) {
    return absl::UnknownError(
        absl::StrCat("getaddrinfo(): ", gai_strerror(err)));
  }
  std::unique_ptr<struct addrinfo, decltype(&freeaddrinfo)>
      address_info_deleter(address_info, freeaddrinfo);

  int socket_fd = 0;
  bool connected = false;
  for (auto* r = address_info; r != nullptr; r = r->ai_next) {
    socket_fd = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
    if (socket_fd == -1) {
      continue;
    }
    connected = connect(socket_fd, r->ai_addr, r->ai_addrlen) != -1;
    if (connected) {
      break;
    }
    close(socket_fd);
  }
  if (!connected) {
    return absl::UnknownError(
        absl::StrCat("Cannot connect to ", server, ":", port));
  }

  absl::Cleanup socked_closer([socket_fd] { close(socket_fd); });

  if (auto packet_size = packet.size();
      send(socket_fd, packet.data(), packet_size, /*flags=*/0) != packet_size) {
    return absl::UnknownError(
        absl::StrCat("Failed to send ", packet_size, " bytes to UI"));
  }
  return absl::OkStatus();
}

absl::Status SendGuiMessage(const Config& config, absl::string_view arguments,
                            absl::FunctionRef<void()> on_retry_callback) {
  const auto& ui_config = config.ui();
  const std::string server =
      !ui_config.server().empty() ? ui_config.server() : "127.0.0.1";
  const uint16_t port = ui_config.port() != 0 ? ui_config.port() : 2000;
  if (DoSendGuiMessageTCP(server, port, arguments).ok()) {
    return absl::OkStatus();
  }
  NA_RETURN_IF_ERROR(StartUiWithOptions(
      /*extra_args=*/{},
      StartUiOptions{}
          .set_java_binary(ui_config.java_binary())
          .set_java_vm_options(ui_config.java_vm_option())
          .set_max_heap_size_mb(ui_config.max_heap_size_mb())
          .set_bindiff_dir(config::GetBinDiffDirOrDefault(config))));

  const int retries = ui_config.retries() != 0 ? ui_config.retries() : 20;
  absl::Status status;
  for (int retry = 0; retry < retries * 10; ++retry) {
    if (status = DoSendGuiMessageTCP(server, port, arguments); status.ok()) {
      return absl::OkStatus();
    }

    absl::SleepFor(absl::Milliseconds(100));
    on_retry_callback();
  }
  return status;
}

}  // namespace security::bindiff
