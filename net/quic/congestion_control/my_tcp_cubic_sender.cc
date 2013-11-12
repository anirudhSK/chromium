// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/congestion_control/my_tcp_cubic_sender.h"

namespace net {

namespace {
const QuicByteCount kMaxSegmentSize = kMaxPacketSize;
const int64 kInitialCongestionWindow = 10;
const int kInitialRttMs = 60;  // At a typical RTT 60 ms.
const float kAlpha = 0.125f;
const float kBeta = 0.25f;
// Sprout-EWMA constants.
const int kUpdateInterval = 20;  // Tick > 20 ms.
const int kSendInterval = 100;   // Avoid driving the drain duration > 100 ms.
const int kMaxSendDelta = 5;
const float kEwmaGain = 0.125f;
};  // namespace

MyTcpCubicSender::MyTcpCubicSender()
    : bytes_in_flight_(0),
      smoothed_rtt_(QuicTime::Delta::Zero()),
      mean_deviation_(QuicTime::Delta::Zero()),
      throughput_(QuicBandwidth::FromKBytesPerSecond(
          kInitialCongestionWindow * kMaxSegmentSize / kSendInterval)),
      last_update_time_(QuicTime::Zero()),
      last_send_time_(QuicTime::Zero()),
      last_receive_time_(QuicTime::Zero()),
      bytes_in_tick_(0) {
  DLOG(INFO) << "Using the MyTCP sender";
}

MyTcpCubicSender::~MyTcpCubicSender() {
}

void MyTcpCubicSender::SetFromConfig(const QuicConfig& config, bool is_server) {
  if (is_server) {
    throughput_ = QuicBandwidth::FromKBytesPerSecond(
        config.server_initial_congestion_window() * kMaxSegmentSize /
        kSendInterval);
  }
}

void MyTcpCubicSender::OnIncomingQuicCongestionFeedbackFrame(
    const QuicCongestionFeedbackFrame& feedback,
    QuicTime feedback_receive_time,
    const SentPacketsMap& sent_packets) {
  DLOG(INFO) << "Received feedback = " << feedback;

  for (TimeMap::const_iterator received_it =
       feedback.my_tcp.received_packet_times.begin();
       received_it != feedback.my_tcp.received_packet_times.end();
       ++received_it) {
    QuicPacketSequenceNumber sequence_number = received_it->first;
    SentPacketsMap::const_iterator sent_it = sent_packets.find(sequence_number);
    if (sent_it == sent_packets.end()) {
      // Too old data; ignore and move forward.
      continue;
    }
    QuicTime time_received = received_it->second;
    QuicTime time_sent = sent_it->second->SendTimestamp();
    QuicByteCount bytes_sent = sent_it->second->BytesSent();

    // Sprout-EWMA logic.
    if (!last_update_time_.IsInitialized()) {
      last_update_time_ = time_received;
    }

    bool force_update = last_send_time_.IsInitialized() &&
        time_sent.Subtract(last_send_time_).ToMilliseconds() > kMaxSendDelta;
    QuicTime::Delta tick_length =
        (force_update ? last_receive_time_ : time_received)
        .Subtract(last_update_time_);
    bytes_in_tick_ += (force_update ? 0 : bytes_sent);

    DLOG_IF(INFO, force_update) << "Send delta = " <<
       time_sent.Subtract(last_send_time_) .ToMilliseconds();

    if (force_update || tick_length.ToMilliseconds() > kUpdateInterval) {
      QuicTime::Delta min_delta = QuicTime::Delta::FromMilliseconds(1);
      if (tick_length < min_delta) tick_length = min_delta;

      DLOG(INFO) << "Tick length = " << tick_length.ToMilliseconds() << " ms";
      DLOG(INFO) << "Bytes in this tick = " << bytes_in_tick_ << " bytes";

      QuicBandwidth current_throughput =
          QuicBandwidth::FromBytesAndTimeDelta(bytes_in_tick_, tick_length);
      // We should use rtt (current measurement) instead of SmoothedRtt, but it
      // seems to be broken and is always equal to infinity.
      QuicBandwidth min_throughput =
          QuicBandwidth::FromBytesAndTimeDelta(kMaxSegmentSize, SmoothedRtt());

      throughput_ = throughput_.Scale(1 - kEwmaGain).Add(
          current_throughput.Scale(kEwmaGain));
      if (throughput_ < min_throughput) {
        throughput_ = min_throughput;
      }

      DLOG(INFO) << "Current throughput = "
          << current_throughput.ToKBytesPerSecond() << " kB/s";
      DLOG(INFO) << "Minimum throughput = "
          << min_throughput.ToKBytesPerSecond() << " kB/s";
      DLOG(INFO) << "Average throughput = "
          << throughput_.ToKBytesPerSecond() << " kB/s";

      bytes_in_tick_ = (force_update ? bytes_sent : 0);
      last_update_time_ = time_received;
    }

    last_send_time_ = time_sent;
    last_receive_time_ = time_received;
  }
}

void MyTcpCubicSender::OnIncomingAck(
    QuicPacketSequenceNumber acked_sequence_number, QuicByteCount acked_bytes,
    QuicTime::Delta rtt) {
  DCHECK_GE(bytes_in_flight_, acked_bytes);
  bytes_in_flight_ -= acked_bytes;
  AckAccounting(rtt);
}

void MyTcpCubicSender::OnIncomingLoss(QuicTime /*ack_receive_time*/) {
}

bool MyTcpCubicSender::OnPacketSent(QuicTime /*sent_time*/,
                                  QuicPacketSequenceNumber sequence_number,
                                  QuicByteCount bytes,
                                  TransmissionType transmission_type,
                                  HasRetransmittableData is_retransmittable) {
  // Only update bytes_in_flight_ for data packets.
  if (is_retransmittable != HAS_RETRANSMITTABLE_DATA) {
    return false;
  }

  bytes_in_flight_ += bytes;
  return true;
}

void MyTcpCubicSender::OnPacketAbandoned(QuicPacketSequenceNumber sequence_number,
                                      QuicByteCount abandoned_bytes) {
  DCHECK_GE(bytes_in_flight_, abandoned_bytes);
  bytes_in_flight_ -= abandoned_bytes;
}

QuicTime::Delta MyTcpCubicSender::TimeUntilSend(
    QuicTime /* now */,
    TransmissionType transmission_type,
    HasRetransmittableData has_retransmittable_data,
    IsHandshake handshake) {
  if (transmission_type == NACK_RETRANSMISSION ||
      has_retransmittable_data == NO_RETRANSMITTABLE_DATA ||
      handshake == IS_HANDSHAKE) {
    // For TCP we can always send an ACK immediately.
    // We also immediately send any handshake packet (CHLO, etc.).  We provide
    // this special dispensation for handshake messages in QUIC, although the
    // concept is not present in TCP.
    return QuicTime::Delta::Zero();
  }
  if (AvailableSendWindow() == 0) {
    return QuicTime::Delta::Infinite();
  }
  return QuicTime::Delta::Zero();
}

QuicByteCount MyTcpCubicSender::AvailableSendWindow() {
  if (bytes_in_flight_ > SendWindow()) {
    return 0;
  }
  return SendWindow() - bytes_in_flight_;
}

QuicByteCount MyTcpCubicSender::SendWindow() {
  QuicByteCount send_window = throughput_.ToBytesPerPeriod(
      QuicTime::Delta::FromMilliseconds(kSendInterval));
  return send_window;
}

QuicByteCount MyTcpCubicSender::GetCongestionWindow() {
  return 0;
}

void MyTcpCubicSender::SetCongestionWindow(QuicByteCount window) {
}

QuicBandwidth MyTcpCubicSender::BandwidthEstimate() {
  return QuicBandwidth::Zero();
}

QuicTime::Delta MyTcpCubicSender::SmoothedRtt() {
  if (smoothed_rtt_.IsZero()) {
    return QuicTime::Delta::FromMilliseconds(kInitialRttMs);
  }
  return smoothed_rtt_;
}

QuicTime::Delta MyTcpCubicSender::RetransmissionDelay() {
  return QuicTime::Delta::FromMicroseconds(
      smoothed_rtt_.ToMicroseconds() + 4 * mean_deviation_.ToMicroseconds());
}

void MyTcpCubicSender::AckAccounting(QuicTime::Delta rtt) {
  if (rtt.IsInfinite() || rtt.IsZero()) {
    return;
  }
  // RTT can't be negative.
  DCHECK_LT(0, rtt.ToMicroseconds());

  // First time call.
  if (smoothed_rtt_.IsZero()) {
    smoothed_rtt_ = rtt;
    mean_deviation_ = QuicTime::Delta::FromMicroseconds(
        rtt.ToMicroseconds() / 2);
  } else {
    mean_deviation_ = QuicTime::Delta::FromMicroseconds(
        (1.0 - kBeta) * mean_deviation_.ToMicroseconds() +
        kBeta * abs(smoothed_rtt_.ToMicroseconds() - rtt.ToMicroseconds()));
    smoothed_rtt_ = QuicTime::Delta::FromMicroseconds(
        (1.0 - kAlpha) * smoothed_rtt_.ToMicroseconds() +
        kAlpha * rtt.ToMicroseconds());
    DLOG(INFO) << "smoothed_rtt_:" << smoothed_rtt_.ToMicroseconds()
               << " mean_deviation_:" << mean_deviation_.ToMicroseconds();
  }
}

}  // namespace net
