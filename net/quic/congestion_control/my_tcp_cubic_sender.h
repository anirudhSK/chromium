// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TCP cubic send side congestion algorithm, emulates the behavior of
// TCP cubic.

#ifndef NET_QUIC_CONGESTION_CONTROL_MY_TCP_CUBIC_SENDER_H_
#define NET_QUIC_CONGESTION_CONTROL_MY_TCP_CUBIC_SENDER_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "net/base/net_export.h"
#include "net/quic/congestion_control/send_algorithm_interface.h"
#include "net/quic/quic_bandwidth.h"
#include "net/quic/quic_protocol.h"
#include "net/quic/quic_time.h"

namespace net {

namespace test {
class MyTcpCubicSenderPeer;
}  // namespace test

class NET_EXPORT_PRIVATE MyTcpCubicSender : public SendAlgorithmInterface {
 public:
  // Reno option and max_tcp_congestion_window are provided for testing.
  MyTcpCubicSender();
  virtual ~MyTcpCubicSender();

  // Start implementation of SendAlgorithmInterface.
  virtual void SetFromConfig(const QuicConfig& config, bool is_server) OVERRIDE;
  virtual void SetMaxPacketSize(QuicByteCount max_packet_size) OVERRIDE;
  virtual void OnIncomingQuicCongestionFeedbackFrame(
      const QuicCongestionFeedbackFrame& feedback,
      QuicTime feedback_receive_time) OVERRIDE;
  virtual void OnPacketAcked(QuicPacketSequenceNumber acked_sequence_number,
                             QuicByteCount acked_bytes) OVERRIDE;
  virtual void OnPacketLost(QuicPacketSequenceNumber sequence_number,
                            QuicTime ack_receive_time) OVERRIDE;
  virtual bool OnPacketSent(
      QuicTime sent_time,
      QuicPacketSequenceNumber sequence_number,
      QuicByteCount bytes,
      TransmissionType transmission_type,
      HasRetransmittableData is_retransmittable) OVERRIDE;
  virtual void OnRetransmissionTimeout(bool packets_retransmitted) OVERRIDE;
  virtual void OnPacketAbandoned(QuicPacketSequenceNumber sequence_number,
                                 QuicByteCount abandoned_bytes) OVERRIDE;
  virtual QuicTime::Delta TimeUntilSend(
      QuicTime now,
      TransmissionType transmission_type,
      HasRetransmittableData has_retransmittable_data,
      IsHandshake handshake) OVERRIDE;
  virtual QuicByteCount GetCongestionWindow() const OVERRIDE;
  virtual void SetCongestionWindow(QuicByteCount window) OVERRIDE;
  virtual QuicBandwidth BandwidthEstimate() const OVERRIDE;
  virtual void UpdateRtt(QuicTime::Delta rtt_sample) OVERRIDE;
  virtual QuicTime::Delta SmoothedRtt() const OVERRIDE;
  virtual QuicTime::Delta RetransmissionDelay() const OVERRIDE;
  // End implementation of SendAlgorithmInterface.

 private:
  class SentPacket {
   public:
    SentPacket(QuicByteCount bytes, QuicTime timestamp)
        : bytes_sent_(bytes),
          send_timestamp_(timestamp) { }
    QuicByteCount bytes_sent() const { return bytes_sent_; }
    const QuicTime& send_timestamp() const { return send_timestamp_; }

   private:
    QuicByteCount bytes_sent_;
    QuicTime send_timestamp_;
  };

  typedef std::map<QuicPacketSequenceNumber, SentPacket*> SentPacketsMap;

  SentPacketsMap packet_history_map_;

  QuicByteCount max_segment_size_;

  // Bytes in flight, aka bytes on the wire.
  QuicByteCount bytes_in_flight_;

  // Smoothed RTT during this session.
  QuicTime::Delta smoothed_rtt_;

  // Mean RTT deviation during this session.
  // Approximation of standard deviation, the error is roughly 1.25 times
  // larger than the standard deviation, for a normally distributed signal.
  QuicTime::Delta mean_deviation_;

  // Sprout-EWMA state.
  QuicBandwidth smoothed_throughput_;
  QuicTime last_update_time_;
  QuicByteCount bytes_in_tick_;
  QuicTime::Delta min_rtt_;
  int64 initial_ticks_left;
  int packets_in_burst_;

  DISALLOW_COPY_AND_ASSIGN(MyTcpCubicSender);
};

}  // namespace net

#endif  // NET_QUIC_CONGESTION_CONTROL_MY_TCP_CUBIC_SENDER_H_
