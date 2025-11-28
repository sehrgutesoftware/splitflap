/*
   Copyright 2021 Scott Bezek and the splitflap contributors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "tcp_task.h"
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "../core/network_config.h"
#include "../core/tcp_stream.h"

TcpTask::TcpTask(SplitflapTask &splitflap_task, const uint8_t task_core,
                 uint16_t port)
    : Task("TCP", 24576, 1, task_core), Logger(),
      splitflap_task_(splitflap_task), port_(port), server_(port) {
  log_queue_ = xQueueCreate(10, sizeof(std::string *));
  assert(log_queue_ != NULL);

  supervisor_state_queue_ = xQueueCreate(1, sizeof(PB_SupervisorState));
  assert(supervisor_state_queue_ != NULL);
}

void TcpTask::run() {
  // Wait for network to be ready before starting the server
  log("Waiting for network connection...");
  while (!NetworkConfig::isConnected()) {
    delay(1000);
  }
  log("Network connected, starting TCP server");

  server_.begin();

  while (1) {
    WiFiClient client = server_.available();
    if (client) {
      handleClient(client);
    }
    delay(100);
  }
}

void TcpTask::handleClient(
    WiFiClient &client) { // Enable TCP keepalive on the socket
  int fd = client.fd();
  if (fd >= 0) {
    int keepAlive = 1;
    int keepIdle = 5;     // Start keepalives after 5 seconds of idle
    int keepInterval = 5; // Send keepalive probes every 5 seconds
    int keepCount = 3;    // Close connection after 3 failed probes

    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
  }

  TcpStream stream(client);

  // Start in legacy protocol mode
  SerialLegacyJsonProtocol *legacy_protocol =
      new SerialLegacyJsonProtocol(splitflap_task_, stream);
  SerialProtoProtocol *proto_protocol = nullptr;

  legacy_protocol->init();
  SerialProtocol *current_protocol = legacy_protocol;

  ProtocolChangeCallback protocol_change_callback =
      [this, &current_protocol, &legacy_protocol, &proto_protocol,
       &stream](uint8_t protocol) {
        switch (protocol) {
        case SERIAL_PROTOCOL_LEGACY:
          if (proto_protocol != nullptr) {
            delete proto_protocol;
            proto_protocol = nullptr;
          }
          if (legacy_protocol == nullptr) {
            legacy_protocol =
                new SerialLegacyJsonProtocol(splitflap_task_, stream);
            legacy_protocol->init();
          }
          current_protocol = legacy_protocol;
          break;
        case SERIAL_PROTOCOL_PROTO:
          if (legacy_protocol != nullptr) {
            delete legacy_protocol;
            legacy_protocol = nullptr;
          }
          if (proto_protocol == nullptr) {
            proto_protocol = new SerialProtoProtocol(splitflap_task_, stream);
          }
          current_protocol = proto_protocol;
          break;
        default:
          log("Unknown protocol requested");
          break;
        }
      };

  if (legacy_protocol != nullptr) {
    legacy_protocol->setProtocolChangeCallback(protocol_change_callback);
  }

  SplitflapState last_state = {};
  while (stream.connected()) {
    SplitflapState new_state = splitflap_task_.getState();

    if (new_state != last_state) {
      current_protocol->handleState(last_state, new_state);
      last_state = new_state;
    }

    current_protocol->loop();

    std::string *log_string;
    while (xQueueReceive(log_queue_, &log_string, 0) == pdTRUE) {
      current_protocol->log(log_string->c_str());
      delete log_string;
    }

    PB_SupervisorState supervisor_state;
    if (xQueueReceive(supervisor_state_queue_, &supervisor_state, 0) ==
        pdTRUE) {
      current_protocol->sendSupervisorState(supervisor_state);
    }
    delay(1);
  }

  // Clean up
  if (legacy_protocol != nullptr) {
    delete legacy_protocol;
  }
  if (proto_protocol != nullptr) {
    delete proto_protocol;
  }

  client.stop();
}

void TcpTask::log(const char *msg) {
  // Allocate a string for the duration it's in the queue; it is free'd by the
  // queue consumer
  std::string *msg_str = new std::string(msg);

  // Put string in queue (or drop if full to avoid blocking)
  xQueueSendToBack(log_queue_, &msg_str, 0);
}

void TcpTask::sendSupervisorState(PB_SupervisorState &supervisor_state) {
  // Only queue the latest supervisor state
  xQueueOverwrite(supervisor_state_queue_, &supervisor_state);
}
