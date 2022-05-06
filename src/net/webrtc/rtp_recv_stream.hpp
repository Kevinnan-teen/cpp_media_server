#ifndef RTP_STREAM_HPP
#define RTP_STREAM_HPP
#include "utils/stream_statics.hpp"
#include "utils/timeex.hpp"
#include "net/rtprtcp/rtp_packet.hpp"
#include "net/rtprtcp/rtcp_sr.hpp"
#include "nack_generator.hpp"
#include "rtc_stream_pub.hpp"
#include "json.hpp"

#include <stdint.h>
#include <stddef.h>
#include <string>

using json = nlohmann::json;

class rtp_recv_stream : public nack_generator_callback_interface
{
public:
    rtp_recv_stream(rtc_stream_callback* cb, std::string media_type,
        uint32_t ssrc, uint8_t payloadtype, bool is_rtx, int clock_rate);
    virtual ~rtp_recv_stream();

public:
    void on_handle_rtp(rtp_packet* pkt);
    void on_handle_rtx_packet(rtp_packet* pkt);
    void on_handle_rtcp_sr(rtcp_sr_packet* sr_pkt);

public:
    virtual void generate_nacklist(const std::vector<uint16_t>& seq_vec) override;
    
public:
    void on_timer(int64_t now_ms);
    void set_rtx_ssrc(uint32_t ssrc) {rtx_ssrc_ = ssrc;}
    void set_rtx_payloadtype(uint8_t type) {rtx_payloadtype_ = type;}
    int64_t get_expected_packets();
    int64_t get_packet_lost();
    void update_rtt(int64_t rtt);
    void get_statics(json& json_data);

private:
    void init_seq(uint16_t seq);
    void update_seq(uint16_t seq);
    void generate_jitter(uint32_t rtp_timestamp, int64_t recv_pkt_ms);

private:
    void send_rtcp_rr();

private:
    rtc_stream_callback* cb_ = nullptr;
    std::string media_type_;
    uint32_t ssrc_           = 0;
    uint32_t rtx_ssrc_       = 0;
    int clock_rate_          = 0;
    uint8_t payloadtype_     = 0;
    uint8_t rtx_payloadtype_ = 0;
    bool has_rtx_            = false;

private:
    stream_statics recv_statics_;
    uint32_t jitter_q4_    = 0;
    uint32_t jitter_       = 0;
    uint32_t lsr_          = 0;
    int64_t last_sr_ms_    = 0;
    int64_t discard_count_ = 0;
    int64_t total_lost_    = 0;
    int64_t expect_recv_   = 0;
    int64_t last_recv_     = 0;
    uint8_t frac_lost_     = 0;
    double lost_percent_   = 0.0;

private:
    int64_t rtt_ = 0;

private:
    bool first_pkt_      = true;
    uint16_t base_seq_   = 0;
    uint16_t max_seq_    = 0;
    uint32_t bad_seq_    = RTP_SEQ_MOD + 1;   /* so seq == bad_seq is false */
    uint32_t cycles_     = 0;
    int64_t last_pkt_ms_ = 0;
    int64_t last_rtp_ts_ = 0;

private://for rtcp sr
    NTP_TIMESTAMP ntp_;//from rtcp sr
    uint32_t sr_ssrc_      = 0;
    int64_t rtp_timestamp_ = 0;
    int64_t sr_local_ms_   = 0;
    uint32_t pkt_count_    = 0;
    uint32_t bytes_count_  = 0;

private:
    nack_generator* nack_handle_ = nullptr;

private:
    int64_t last_statics_ts_   = 0;
    int64_t last_rtcp_send_ts_ = 0;
};

#endif
