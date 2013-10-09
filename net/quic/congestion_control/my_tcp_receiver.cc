// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/basictypes.h"
#include "net/quic/congestion_control/my_tcp_receiver.h"

namespace net {

// Originally 64K bytes for TCP, setting it to 256K to support higher bitrates.
const QuicByteCount kReceiveWindowTCP = 256000;

MyTcpReceiver::MyTcpReceiver()
    : accumulated_number_of_recoverd_lost_packets_(0),
      receive_window_(kReceiveWindowTCP) {
  DLOG(INFO) << "Using the MyTCP receiver";
}

bool MyTcpReceiver::GenerateCongestionFeedback(
    QuicCongestionFeedbackFrame* feedback) {
  feedback->type = kMyTCP;
  feedback->my_tcp.accumulated_number_of_lost_packets =
      accumulated_number_of_recoverd_lost_packets_;
  feedback->my_tcp.receive_window = receive_window_;
  return true;
}

void MyTcpReceiver::RecordIncomingPacket(QuicByteCount bytes,
                                       QuicPacketSequenceNumber sequence_number,
                                       QuicTime timestamp,
                                       bool revived) {
  if (revived) {
    ++accumulated_number_of_recoverd_lost_packets_;
  }
}

}  // namespace net
