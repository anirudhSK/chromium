// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/congestion_control/receive_algorithm_interface.h"

#include "net/quic/congestion_control/fix_rate_receiver.h"
#include "net/quic/congestion_control/inter_arrival_receiver.h"
#include "net/quic/congestion_control/my_tcp_receiver.h"
#include "net/quic/congestion_control/tcp_receiver.h"

namespace net {

// Factory for receive side congestion control algorithm.
ReceiveAlgorithmInterface* ReceiveAlgorithmInterface::Create(
    CongestionFeedbackType type) {
  switch (type) {
    case kTCP:
      return new TcpReceiver();
    case kInterArrival:
      return new InterArrivalReceiver();
    case kFixRate:
      return new FixRateReceiver();
    case kMyTCP:
      return new MyTcpReceiver();
  }
  return NULL;
}

}  // namespace net
