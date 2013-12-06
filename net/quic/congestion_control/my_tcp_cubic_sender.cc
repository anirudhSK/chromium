// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/congestion_control/my_tcp_cubic_sender.h"

namespace net {

namespace {
const int64 kInitialCongestionWindow = 10;
const int kInitialRttMs = 60;  // At a typical RTT 60 ms.
const float kAlpha = 0.125f;
const float kBeta = 0.25f;

// Sprout-EWMA constants.
const int64 kTargetDelayMs = 100;
const int64 kMinTickLengthMs = 20;
const int64 kMaxTimeToNextMs = 5;
const float kGamma = 0.125f;
};  // namespace

MyTcpCubicSender::MyTcpCubicSender()
    : max_segment_size_(kMaxPacketSize),
      bytes_in_flight_(0),
      smoothed_rtt_(QuicTime::Delta::Zero()),
      mean_deviation_(QuicTime::Delta::Zero()),
      smoothed_throughput_(QuicBandwidth::FromKBytesPerSecond(
          kInitialCongestionWindow * kMaxPacketSize / kTargetDelayMs)),
      last_update_time_(QuicTime::Zero()),
      bytes_in_tick_(0),
      min_rtt_(QuicTime::Delta::Zero()) {
  DVLOG(1) << "Using the Sprout-EWMA sender";
}

MyTcpCubicSender::~MyTcpCubicSender() {
}

void MyTcpCubicSender::SetFromConfig(const QuicConfig& config, bool is_server) {
  if (is_server) {
    smoothed_throughput_ = QuicBandwidth::FromKBytesPerSecond(
        config.server_initial_congestion_window() * max_segment_size_ /
        kTargetDelayMs);
  }
}

void MyTcpCubicSender::SetMaxPacketSize(QuicByteCount max_packet_size) {
  max_segment_size_ = max_packet_size;
}

void MyTcpCubicSender::OnIncomingQuicCongestionFeedbackFrame(
    const QuicCongestionFeedbackFrame& feedback,
    QuicTime feedback_receive_time,
    const SentPacketsMap& sent_packets) {
  for (TimeMap::const_iterator received_it =
       feedback.my_tcp.received_packet_times.begin();
       received_it != feedback.my_tcp.received_packet_times.end();
       ++received_it) {
    const QuicPacketSequenceNumber sequence_number = received_it->first;
    SentPacketsMap::const_iterator sent_it = sent_packets.find(sequence_number);
    if (sent_it == sent_packets.end()) {
      // Too old data; ignore and move forward.
      continue;
    }
    const QuicTime time_received = received_it->second;
    const QuicTime time_sent = sent_it->second->send_timestamp();
    const QuicByteCount bytes_sent = sent_it->second->bytes_sent();

    bool force_update = false;
    sent_it++;
    if (sent_it == sent_packets.end()) {
      force_update = true;
    } else {
      const QuicTime time_next_sent = sent_it->second->send_timestamp();
      force_update = time_next_sent.Subtract(time_sent).ToMilliseconds() >
          kMaxTimeToNextMs;
    }

    // Sprout-EWMA logic.
    if (!last_update_time_.IsInitialized()) {
      last_update_time_ = time_received;
    }

    const QuicTime::Delta tick_length = force_update ?
        QuicTime::Delta::FromMilliseconds(kMinTickLengthMs) :
        time_received.Subtract(last_update_time_);
    bytes_in_tick_ += bytes_sent;

    if (tick_length.ToMilliseconds() >= kMinTickLengthMs) {
      QuicBandwidth throughput =
          QuicBandwidth::FromBytesAndTimeDelta(bytes_in_tick_, tick_length);
      smoothed_throughput_ = smoothed_throughput_.Scale(1.0 - kGamma).Add(
          throughput.Scale(kGamma));

      DVLOG(1) << "tick length = " << tick_length.ToMilliseconds() << " ms";
      DVLOG(1) << "bytes in this tick = " << bytes_in_tick_;
      DVLOG(1) << "throughput = " << throughput.ToKBytesPerSecond() << " kB/s";
      DVLOG(1) << "smoothed throughput = "
               << smoothed_throughput_.ToKBytesPerSecond() << " kB/s";

      bytes_in_tick_ = 0;
      last_update_time_ = force_update ? QuicTime::Zero() : time_received;
    }
  }
}

void MyTcpCubicSender::OnPacketAcked(
    QuicPacketSequenceNumber acked_sequence_number, QuicByteCount acked_bytes,
    QuicTime::Delta rtt) {
  DCHECK_GE(bytes_in_flight_, acked_bytes);
  bytes_in_flight_ -= acked_bytes;
  AckAccounting(rtt);
}


void MyTcpCubicSender::OnPacketLost(
    QuicPacketSequenceNumber /*sequence_number*/,
    QuicTime /*ack_receive_time*/) {
}

bool MyTcpCubicSender::OnPacketSent(
    QuicTime /*sent_time*/, QuicPacketSequenceNumber sequence_number,
    QuicByteCount bytes, TransmissionType transmission_type,
    HasRetransmittableData is_retransmittable) {
  // Only update bytes_in_flight_ for data packets.
  if (is_retransmittable != HAS_RETRANSMITTABLE_DATA) {
    return false;
  }

  bytes_in_flight_ += bytes;
  return true;
}

void MyTcpCubicSender::OnRetransmissionTimeout() {
}

void MyTcpCubicSender::OnPacketAbandoned(
    QuicPacketSequenceNumber sequence_number, QuicByteCount abandoned_bytes) {
  DCHECK_GE(bytes_in_flight_, abandoned_bytes);
  bytes_in_flight_ -= abandoned_bytes;
}

QuicTime::Delta MyTcpCubicSender::TimeUntilSend(
    QuicTime /*now*/, TransmissionType transmission_type,
    HasRetransmittableData has_retransmittable_data, IsHandshake handshake) {
  if (transmission_type == NACK_RETRANSMISSION ||
      has_retransmittable_data == NO_RETRANSMITTABLE_DATA ||
      handshake == IS_HANDSHAKE) {
    // For TCP we can always send an ACK immediately.
    // We also immediately send any handshake packet (CHLO, etc.).  We provide
    // this special dispensation for handshake messages in QUIC, although the
    // concept is not present in TCP.
    return QuicTime::Delta::Zero();
  }
  return GetCongestionWindow() > bytes_in_flight_ ? QuicTime::Delta::Zero() :
      QuicTime::Delta::Infinite();
}

QuicByteCount MyTcpCubicSender::GetCongestionWindow() const {
  const int64 target_delay_ms = std::max(kTargetDelayMs,
      min_rtt_.ToMilliseconds());
  return smoothed_throughput_.ToBytesPerPeriod(
      QuicTime::Delta::FromMilliseconds(target_delay_ms));
}

void MyTcpCubicSender::SetCongestionWindow(QuicByteCount window) {
  smoothed_throughput_ = QuicBandwidth::FromKBytesPerSecond(
      window / kTargetDelayMs);
}

QuicBandwidth MyTcpCubicSender::BandwidthEstimate() const {
  return smoothed_throughput_;
}

QuicTime::Delta MyTcpCubicSender::SmoothedRtt() const {
  return smoothed_rtt_.IsZero() ?
      QuicTime::Delta::FromMilliseconds(kInitialRttMs) : smoothed_rtt_;
}

QuicTime::Delta MyTcpCubicSender::RetransmissionDelay() const {
  return QuicTime::Delta::FromMicroseconds(
      smoothed_rtt_.ToMicroseconds() + 4 * mean_deviation_.ToMicroseconds());
}

void MyTcpCubicSender::AckAccounting(QuicTime::Delta rtt) {
  if (rtt.IsInfinite() || rtt.IsZero()) {
    return;
  }

  if (smoothed_rtt_.IsZero()) {  // First time call.
    smoothed_rtt_ = rtt;
    mean_deviation_ = QuicTime::Delta::FromMicroseconds(
        rtt.ToMicroseconds() / 2);

    min_rtt_ = rtt;
  } else {
    mean_deviation_ = QuicTime::Delta::FromMicroseconds(
        (1.0 - kBeta) * mean_deviation_.ToMicroseconds() +
        kBeta * abs(smoothed_rtt_.ToMicroseconds() - rtt.ToMicroseconds()));
    smoothed_rtt_ = QuicTime::Delta::FromMicroseconds(
        (1.0 - kAlpha) * smoothed_rtt_.ToMicroseconds() +
        kAlpha * rtt.ToMicroseconds());
    DVLOG(1) << "smoothed_rtt_:" << smoothed_rtt_.ToMicroseconds()
             << " mean_deviation_:" << mean_deviation_.ToMicroseconds();

    if (min_rtt_ > rtt) {
      min_rtt_ = rtt;
    }
  }
}

}  // namespace net
