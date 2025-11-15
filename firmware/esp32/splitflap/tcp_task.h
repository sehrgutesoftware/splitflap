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
#pragma once

#include "config.h"

#ifdef USE_ETHERNET
#include <ETH.h>
#else
#include <WiFi.h>
#endif

#include "../core/network_config.h"
#include "../core/splitflap_task.h"
#include "../core/task.h"
#include "../core/tcp_stream.h"

#include "serial_legacy_json_protocol.h"
#include "serial_proto_protocol.h"

class TcpTask : public Task<TcpTask>, public Logger {
  friend class Task<TcpTask>; // Allow base Task to invoke protected run()

public:
  TcpTask(SplitflapTask &splitflap_task, const uint8_t task_core,
          uint16_t port = 9000);
  virtual ~TcpTask() {};

  void log(const char *msg) override;

  void sendSupervisorState(PB_SupervisorState &supervisor_state);

protected:
  void run();

private:
  SplitflapTask &splitflap_task_;
  uint16_t port_;
  WiFiServer server_;

  QueueHandle_t log_queue_;
  QueueHandle_t supervisor_state_queue_;

  void handleClient(WiFiClient &client);
};
