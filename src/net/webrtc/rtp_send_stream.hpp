#ifndef RTP_SEND_STREAM_HPP
#define RTP_SEND_STREAM_HPP

#include "net/rtprtcp/rtp_packet.hpp"
#include "net/rtprtcp/rtcp_sr.hpp"
#include "net/rtprtcp/rtcpfb_nack.hpp"
#include "net/rtprtcp/rtcp_rr.hpp"
#include "utils/stream_statics.hpp"
#include "utils/timeex.hpp"
#include "rtc_stream_pub.hpp"
#include "json.hpp"
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <map>

using json = nlohmann::json;

typedef struct NACK_PACKET_S {
    uint32_t last_sent_timestamp;
    int sent_count;
    rtp_packet* packet;
} NACK_PACKET;

class rtp_send_stream
{
public:
    rtp_send_stream(const std::string& media_type, bool nack_enable, rtc_stream_callback* cb);
    ~rtp_send_stream();

public:
    void set_rtp_payload_type(uint8_t payload_type) {rtp_payload_type_ = payload_type;}
    uint8_t get_rtp_payload_type() { return rtp_payload_type_; }

    void set_rtx_payload_type(uint8_t payload_type) {rtx_payload_type_ = payload_type;}
    uint8_t get_rtx_payload_type() { return rtx_payload_type_; }
    
    void set_clock(int clock) { clock_ = clock; }
    int get_clock() { return clock_; }

    void set_rtp_ssrc(uint32_t ssrc) { rtp_ssrc_ = ssrc; }
    uint32_t get_rtp_ssrc() { return rtp_ssrc_; }

    void set_rtx_ssrc(uint32_t ssrc) { rtx_ssrc_ = ssrc; }
    uint32_t get_rtx_ssrc() { return rtx_ssrc_; }

    void get_statics(json& json_data);

public:
    void on_timer(int64_t now_ms);

public:
    void on_send_rtp_packet(rtp_packet* pkt);
    void handle_fb_rtp_nack(rtcp_fb_nack* nack_pkt);
    void handle_rtcp_rr(rtcp_rr_packet* rr_pkt);

    rtcp_sr_packet* get_rtcp_sr(int64_t now_ms);

private:
    void save_buffer(rtp_packet* pkt);
    void clear_buffer();

private:
    std::string media_type_;
    uint32_t rtp_ssrc_        = 0;
    uint32_t rtx_ssrc_        = 0;
    uint8_t rtp_payload_type_ = 0;
    uint8_t rtx_payload_type_ = 0;
    int clock_                = 0;
    bool nack_enable_         = false;
    rtc_stream_callback* cb_  = nullptr;

private:
    std::map<uint16_t, NACK_PACKET> rtp_buffer_map_;
    uint16_t first_seq_ = 0;

private:
    stream_statics send_statics_;
    int64_t last_statics_ms_ = 0;

private:
    uint32_t last_sr_rtp_ts_ = 0;
    NTP_TIMESTAMP last_sr_ntp_ts_;
    float rtt_     = RTT_DEFAULT * 1.0;
    float avg_rtt_ = 0;

private://for rtcp rr
    float lost_rate_     = 0.0;
    uint32_t lost_total_ = 0;
    uint32_t jitter_     = 0;

private:
    int64_t last_send_rtcp_ts_ = 0;
};

#endif