// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/basictypes.h"
#include "net/quic/congestion_control/my_tcp_receiver.h"

namespace net {

// Originally 64K bytes for TCP, setting it to 256K to support higher bitrates.
const QuicByteCount kReceiveWindowTCP = 256000;
const size_t kMaxTimeMapSize = 8;

MyTcpReceiver::MyTcpReceiver()
    : accumulated_number_of_recoverd_lost_packets_(0),
      receive_window_(kReceiveWindowTCP) {
  DVLOG(1) << "Using the MyTCP receiver";
}

bool MyTcpReceiver::GenerateCongestionFeedback(
    QuicCongestionFeedbackFrame* feedback) {
  /*
  // TODO(somakrdas): Not making this optimization yet.

  if (received_packet_times_.size() <= 1) {
    // Don't waste resources by sending a feedback frame for only one packet.
    return false;
  }
  */

  feedback->type = kMyTCP;
  // TODO(somakrdas): Is this necessary?
  feedback->my_tcp.accumulated_number_of_lost_packets =
      accumulated_number_of_recoverd_lost_packets_;
  feedback->my_tcp.receive_window = receive_window_;

  // Copy our current receive set to our feedback message, we will not resend
  // this data if it is lost.
  feedback->my_tcp.received_packet_times = received_packet_times_;

  // Prepare for the next set of arriving packets by clearing our current set.
  received_packet_times_.clear();

  return true;
}

void MyTcpReceiver::RecordIncomingPacket(QuicByteCount bytes,
                                       QuicPacketSequenceNumber sequence_number,
                                       QuicTime timestamp) {
  received_packet_times_.insert(std::make_pair(sequence_number, timestamp));

  if (received_packet_times_.size() > kMaxTimeMapSize) {
    TimeMap::iterator it = received_packet_times_.upper_bound(sequence_number -
        static_cast<QuicPacketSequenceNumber>(kMaxTimeMapSize));
    received_packet_times_.erase(received_packet_times_.begin(), it);
  }
}

}  // namespace net
