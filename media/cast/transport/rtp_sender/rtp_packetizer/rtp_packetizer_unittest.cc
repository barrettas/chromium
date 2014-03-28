// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cast/transport/rtp_sender/rtp_packetizer/rtp_packetizer.h"

#include "base/memory/scoped_ptr.h"
#include "base/test/simple_test_tick_clock.h"
#include "media/cast/logging/simple_event_subscriber.h"
#include "media/cast/test/fake_single_thread_task_runner.h"
#include "media/cast/transport/pacing/paced_sender.h"
#include "media/cast/transport/rtp_sender/packet_storage/packet_storage.h"
#include "media/cast/transport/rtp_sender/rtp_packetizer/test/rtp_header_parser.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace media {
namespace cast {
namespace transport {

namespace {
static const int kPayload = 127;
static const uint32 kTimestampMs = 10;
static const uint16 kSeqNum = 33;
static const int kMaxPacketLength = 1500;
static const int kSsrc = 0x12345;
static const unsigned int kFrameSize = 5000;
static const int kMaxPacketStorageTimeMs = 300;
static const uint32 kStartFrameId = GG_UINT32_C(0xffffffff);
}

class TestRtpPacketTransport : public PacketSender {
 public:
  explicit TestRtpPacketTransport(RtpPacketizerConfig config)
      : config_(config),
        sequence_number_(kSeqNum),
        packets_sent_(0),
        expected_number_of_packets_(0),
        expected_packet_id_(0),
        expected_frame_id_(0),
        expectd_rtp_timestamp_(0) {}

  void VerifyRtpHeader(const RtpCastTestHeader& rtp_header) {
    VerifyCommonRtpHeader(rtp_header);
    VerifyCastRtpHeader(rtp_header);
  }

  void VerifyCommonRtpHeader(const RtpCastTestHeader& rtp_header) {
    EXPECT_EQ(kPayload, rtp_header.payload_type);
    EXPECT_EQ(sequence_number_, rtp_header.sequence_number);
    EXPECT_EQ(expectd_rtp_timestamp_, rtp_header.rtp_timestamp);
    EXPECT_EQ(config_.ssrc, rtp_header.ssrc);
    EXPECT_EQ(0, rtp_header.num_csrcs);
  }

  void VerifyCastRtpHeader(const RtpCastTestHeader& rtp_header) {
    EXPECT_FALSE(rtp_header.is_key_frame);
    EXPECT_EQ(expected_frame_id_, rtp_header.frame_id);
    EXPECT_EQ(expected_packet_id_, rtp_header.packet_id);
    EXPECT_EQ(expected_number_of_packets_ - 1, rtp_header.max_packet_id);
    EXPECT_TRUE(rtp_header.is_reference);
    EXPECT_EQ(expected_frame_id_ - 1u, rtp_header.reference_frame_id);
  }

  virtual bool SendPacket(const transport::Packet& packet) OVERRIDE {
    ++packets_sent_;
    RtpHeaderParser parser(packet.data(), packet.size());
    RtpCastTestHeader rtp_header;
    parser.Parse(&rtp_header);
    VerifyRtpHeader(rtp_header);
    ++sequence_number_;
    ++expected_packet_id_;
    return true;
  }

  int number_of_packets_received() const { return packets_sent_; }

  void set_expected_number_of_packets(int expected_number_of_packets) {
    expected_number_of_packets_ = expected_number_of_packets;
  }

  void set_rtp_timestamp(uint32 rtp_timestamp) {
    expectd_rtp_timestamp_ = rtp_timestamp;
  }

  RtpPacketizerConfig config_;
  uint32 sequence_number_;
  int packets_sent_;
  int number_of_packets_;
  int expected_number_of_packets_;
  // Assuming packets arrive in sequence.
  int expected_packet_id_;
  uint32 expected_frame_id_;
  uint32 expectd_rtp_timestamp_;

  DISALLOW_COPY_AND_ASSIGN(TestRtpPacketTransport);
};

class RtpPacketizerTest : public ::testing::Test {
 protected:
  RtpPacketizerTest()
      : task_runner_(new test::FakeSingleThreadTaskRunner(&testing_clock_)),
        video_frame_(),
        packet_storage_(&testing_clock_, kMaxPacketStorageTimeMs) {
    logging_.AddRawEventSubscriber(&subscriber_);
    config_.sequence_number = kSeqNum;
    config_.ssrc = kSsrc;
    config_.payload_type = kPayload;
    config_.max_payload_length = kMaxPacketLength;
    transport_.reset(new TestRtpPacketTransport(config_));
    pacer_.reset(new PacedSender(
        &testing_clock_, &logging_, transport_.get(), task_runner_));
    pacer_->RegisterVideoSsrc(config_.ssrc);
    rtp_packetizer_.reset(new RtpPacketizer(
        pacer_.get(), &packet_storage_, config_, &testing_clock_, &logging_));
    video_frame_.key_frame = false;
    video_frame_.frame_id = 0;
    video_frame_.last_referenced_frame_id = kStartFrameId;
    video_frame_.data.assign(kFrameSize, 123);
    video_frame_.rtp_timestamp =
        GetVideoRtpTimestamp(testing_clock_.NowTicks());
  }

  virtual ~RtpPacketizerTest() {
    logging_.RemoveRawEventSubscriber(&subscriber_);
  }

  void RunTasks(int during_ms) {
    for (int i = 0; i < during_ms; ++i) {
      // Call process the timers every 1 ms.
      testing_clock_.Advance(base::TimeDelta::FromMilliseconds(1));
      task_runner_->RunTasks();
    }
  }

  base::SimpleTestTickClock testing_clock_;
  scoped_refptr<test::FakeSingleThreadTaskRunner> task_runner_;
  EncodedVideoFrame video_frame_;
  PacketStorage packet_storage_;
  RtpPacketizerConfig config_;
  scoped_ptr<TestRtpPacketTransport> transport_;
  LoggingImpl logging_;
  SimpleEventSubscriber subscriber_;
  scoped_ptr<PacedSender> pacer_;
  scoped_ptr<RtpPacketizer> rtp_packetizer_;

  DISALLOW_COPY_AND_ASSIGN(RtpPacketizerTest);
};

TEST_F(RtpPacketizerTest, SendStandardPackets) {
  int expected_num_of_packets = kFrameSize / kMaxPacketLength + 1;
  transport_->set_expected_number_of_packets(expected_num_of_packets);
  transport_->set_rtp_timestamp(video_frame_.rtp_timestamp);

  base::TimeTicks time;
  time += base::TimeDelta::FromMilliseconds(kTimestampMs);
  rtp_packetizer_->IncomingEncodedVideoFrame(&video_frame_, time);
  RunTasks(33 + 1);
  EXPECT_EQ(expected_num_of_packets, transport_->number_of_packets_received());
  std::vector<PacketEvent> packet_events;
  subscriber_.GetPacketEventsAndReset(&packet_events);
  int expected_num_video_sent_to_pacer_count = expected_num_of_packets;
  int num_video_sent_to_pacer_count = 0;
  for (std::vector<PacketEvent>::iterator it = packet_events.begin();
       it != packet_events.end();
       ++it) {
    if (it->type == kVideoPacketSentToPacer)
      num_video_sent_to_pacer_count++;
  }
  EXPECT_EQ(expected_num_video_sent_to_pacer_count,
            num_video_sent_to_pacer_count);
}

TEST_F(RtpPacketizerTest, Stats) {
  EXPECT_FALSE(rtp_packetizer_->send_packets_count());
  EXPECT_FALSE(rtp_packetizer_->send_octet_count());
  // Insert packets at varying lengths.
  int expected_num_of_packets = kFrameSize / kMaxPacketLength + 1;
  transport_->set_expected_number_of_packets(expected_num_of_packets);
  transport_->set_rtp_timestamp(video_frame_.rtp_timestamp);

  testing_clock_.Advance(base::TimeDelta::FromMilliseconds(kTimestampMs));
  rtp_packetizer_->IncomingEncodedVideoFrame(&video_frame_,
                                             testing_clock_.NowTicks());
  RunTasks(33 + 1);
  EXPECT_EQ(expected_num_of_packets, rtp_packetizer_->send_packets_count());
  EXPECT_EQ(kFrameSize, rtp_packetizer_->send_octet_count());
  EXPECT_EQ(expected_num_of_packets, transport_->number_of_packets_received());
}

}  // namespace transport
}  // namespace cast
}  // namespace media
