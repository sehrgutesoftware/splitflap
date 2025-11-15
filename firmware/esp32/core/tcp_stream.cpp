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

#include "tcp_stream.h"

TcpStream::TcpStream(WiFiClient &client) : Stream(), client_(client) {}

int TcpStream::peek() { return client_.peek(); }

int TcpStream::available() { return client_.available(); }

int TcpStream::read() { return client_.read(); }

void TcpStream::flush() { client_.flush(); }

size_t TcpStream::write(uint8_t b) { return client_.write(b); }

size_t TcpStream::write(const uint8_t *buffer, size_t size) {
  return client_.write(buffer, size);
}

bool TcpStream::connected() { return client_.connected(); }
