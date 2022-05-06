#include "rtp_recv_stream.hpp"
#include "timeex.hpp"
#include "net/rtprtcp/rtcp_rr.hpp"
#include "net/rtprtcp/rtcpfb_nack.hpp"
#include "utils/byte_crypto.hpp"
#include "json.hpp"
#include <sstream>

using json = nlohmann::json;

rtp_recv_stream::rtp_recv_stream(rtc_stream_callback* cb, std::string media_type, uint32_t ssrc, uint8_t payloadtype,
                    bool is_rtx, int clock_rate):cb_(cb)
        , media_type_(media_type)
        , ssrc_(ssrc)
        , clock_rate_(clock_rate)
        , payloadtype_(payloadtype)
        , has_rtx_(is_rtx)
{
    memset(&ntp_, 0, sizeof(NTP_TIMESTAMP));
    if (media_type == "video") {
        nack_handle_ = new nack_generator(this);
    } else {
        nack_handle_ = nullptr;
    }
    
    log_infof("rtp stream construct media type:%s, ssrc:%u, payload type:%d, is rtx:%d, clock rate:%d",
        media_type.c_str(), ssrc, payloadtype, is_rtx, clock_rate);
}

rtp_recv_stream::~rtp_recv_stream()
{
    if (nack_handle_) {
        delete nack_handle_;
        nack_handle_ = nullptr;
    }
    log_infof("rtp stream destruct media type:%s, ssrc:%u, payload type:%d, is rtx:%d, clock rate:%d",
        media_type_.c_str(), ssrc_, payloadtype_, has_rtx_, clock_rate_);
}

//rfc3550: A.1 RTP Data Header Validity Checks
void rtp_recv_stream::init_seq(uint16_t seq) {
    base_seq_ = seq;
    max_seq_  = seq;
    bad_seq_  = RTP_SEQ_MOD + 1;   /* so seq == bad_seq is false */
    cycles_   = 0;
}

//rfc3550: A.1 RTP Data Header Validity Checks
void rtp_recv_stream::update_seq(uint16_t seq) {
    const int MAX_DROPOUT    = 3000;
    const int MAX_MISORDER   = 100;
    //const int MIN_SEQUENTIAL = 2;

    uint16_t udelta = seq - max_seq_;

    if (udelta < MAX_DROPOUT) {
        /* in order, with permissible gap */
        if (seq < max_seq_) {
            /*
             * Sequence number wrapped - count another 64K cycle.
             */
            cycles_ += RTP_SEQ_MOD;
        }
        max_seq_ = seq;
    } else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
           /* the sequence number made a very large jump */
           if (seq == bad_seq_) {
               /*
                * Two sequential packets -- assume that the other side
                * restarted without telling us so just re-sync
                * (i.e., pretend this was the first packet).
                */
               init_seq(seq);
           }
           else {
               bad_seq_= (seq + 1) & (RTP_SEQ_MOD-1);
               discard_count_++;
               return;
           }
    } else {
        /* duplicate or reordered packet */
    }
}

int64_t rtp_recv_stream::get_expected_packets() {
    return cycles_ + max_seq_ - bad_seq_ + 1;
}

void rtp_recv_stream::on_timer(int64_t now_ms) {
    const int64_t STATICS_TIMEOUT    = 2500;
    const int64_t VIDEO_RTCP_TIMEOUT = 400;
    const int64_t AUDIO_RTCP_TIMEOUT = 2000;

    if (last_statics_ts_ == 0) {
        last_statics_ts_ = now_ms;
    } else {
        if ((now_ms - last_statics_ts_) > STATICS_TIMEOUT) {
            size_t fps;
            size_t speed = recv_statics_.bytes_per_second((int64_t)now_millisec(), fps);

            last_statics_ts_ = now_ms;
        
            log_debugf("rtc receive mediatype:%s, ssrc:%u, payloadtype:%d, is_rtx:%d, speed(bytes/s):%lu, fps:%lu",
                media_type_.c_str(), ssrc_, payloadtype_, has_rtx_, speed, fps);
        }
    }

    if (last_rtcp_send_ts_ == 0) {
        last_rtcp_send_ts_ = now_ms;
    } else {
        bool rtcp_flag = false;
        uint32_t rand_number = byte_crypto::get_random_uint(5, 15);
        float ratio = rand_number / 10.0;

        if (media_type_ == "video") {
            rtcp_flag = ((now_ms - last_rtcp_send_ts_) * ratio > VIDEO_RTCP_TIMEOUT);
        } else {
            rtcp_flag = ((now_ms - last_rtcp_send_ts_) * ratio > AUDIO_RTCP_TIMEOUT);
        }
        if (rtcp_flag) {
            last_rtcp_send_ts_ = now_ms;
            send_rtcp_rr();
        }
    }
}

void rtp_recv_stream::on_handle_rtcp_sr(rtcp_sr_packet* sr_pkt) {
    int64_t now_ms = now_millisec();

    sr_ssrc_       = sr_pkt->get_ssrc();
    ntp_.ntp_sec   = sr_pkt->get_ntp_sec();
    ntp_.ntp_frac  = sr_pkt->get_ntp_frac();
    rtp_timestamp_ = (int64_t)sr_pkt->get_rtp_timestamp();
    sr_local_ms_   = now_ms;
    pkt_count_     = sr_pkt->get_pkt_count();
    bytes_count_   = sr_pkt->get_bytes_count();

    last_sr_ms_ = now_ms;
    lsr_ = ((ntp_.ntp_sec & 0xffff) << 16) | (ntp_.ntp_frac & 0xffff);
}

void rtp_recv_stream::send_rtcp_rr() {
    rtcp_rr_packet* rr = new rtcp_rr_packet();
    uint32_t highest_seq = (uint32_t)(max_seq_ + cycles_);
    uint32_t dlsr = 0;
    size_t data_len = 0;

    if (last_sr_ms_ > 0) {
        double diff_t = (double)(now_millisec() - last_sr_ms_);
        double dlsr_float = diff_t / 1000 * 65535;

        dlsr = (uint32_t)dlsr_float;
        //log_infof("send_rtcp_rr ssrc:%u, diff_t:%f, dlsr_float:%f, dlsr:%u",
        //    ssrc_, diff_t, dlsr_float, dlsr);
    }
    
    (void)get_packet_lost();

    rr->set_reporter_ssrc(1);
    rr->set_reportee_ssrc(ssrc_);
    rr->set_fraclost(frac_lost_);
    rr->set_cumulative_lost(total_lost_);
    rr->set_highest_seq(highest_seq);
    rr->set_jitter(jitter_);
    rr->set_lsr(lsr_);
    rr->set_dlsr(dlsr);

    uint8_t* data = rr->get_data(data_len);

    //log_infof("send media type(%s) %s", media_type_.c_str(), rr->dump().c_str());

    cb_->stream_send_rtcp(data, data_len);

    delete rr;
}

int64_t rtp_recv_stream::get_packet_lost() {
    int64_t expected = get_expected_packets();
    int64_t recv_count = (int64_t)recv_statics_.get_count();

    int64_t expected_interval = expected - expect_recv_;
    expect_recv_ = expected;

    int64_t recv_interval = recv_count - last_recv_;
    if (last_recv_ <= 0) {
        last_recv_ = recv_count;
        return 0;
    }
    last_recv_ = recv_count;

    if ((expected_interval <= 0) || (recv_interval <= 0)) {
        frac_lost_ = 0;
    } else {
        //log_infof("expected_interval:%ld, recv_interval:%ld, ssrc:%u, media:%s",
        //    expected_interval, recv_interval, ssrc_, media_type_.c_str());
        total_lost_ += expected_interval - recv_interval;
        frac_lost_ = std::round((double)((expected_interval - recv_interval) * 256) / expected_interval);
        lost_percent_ = (expected_interval - recv_interval) / expected_interval;
    }

    return total_lost_;
}

void rtp_recv_stream::generate_jitter(uint32_t rtp_timestamp, int64_t recv_pkt_ms) {
    if (clock_rate_ <= 0) {
        MS_THROW_ERROR("clock rate(%d) is invalid", clock_rate_);
    }
    if ((last_pkt_ms_ == 0) || (last_rtp_ts_ == 0)) {
        return;
    }
    int64_t receive_diff_ms = recv_pkt_ms - last_pkt_ms_;
    uint32_t receive_diff_rtp = static_cast<uint32_t>((receive_diff_ms * clock_rate_) / 1000);
    int32_t time_diff_samples = receive_diff_rtp - (rtp_timestamp - last_rtp_ts_);
    time_diff_samples = std::abs(time_diff_samples);

    // lib_jingle sometimes deliver crazy jumps in TS for the same stream.
    // If this happens, don't update jitter value. Use 5 secs video frequency
    // as the threshold.
    if (time_diff_samples < 450000) {
        // Note we calculate in Q4 to avoid using float.
        int32_t jitter_diff_q4 = (time_diff_samples << 4) - jitter_q4_;
        jitter_q4_ += ((jitter_diff_q4 + 8) >> 4);
        jitter_ = jitter_q4_ >> 4;
    }
    return;
}

void rtp_recv_stream::on_handle_rtp(rtp_packet* pkt) {
    recv_statics_.update(pkt->get_data_length(), pkt->get_local_ms());

    if (first_pkt_) {
        init_seq(pkt->get_seq());
        first_pkt_ = false;
    } else {
        update_seq(pkt->get_seq());
    }

    generate_jitter(pkt->get_timestamp(), pkt->get_local_ms());
    last_pkt_ms_ = pkt->get_local_ms();
    last_rtp_ts_ = pkt->get_timestamp();

    if (media_type_ == "video" && nack_handle_) {
        log_debugf("receive video pkt ssrc:%u, seq:%d", pkt->get_ssrc(), pkt->get_seq());
        nack_handle_->update_nacklist(pkt);
    }
    return;
}

void rtp_recv_stream::on_handle_rtx_packet(rtp_packet* pkt) {
    recv_statics_.update(pkt->get_data_length(), pkt->get_local_ms());
    pkt->rtx_demux(ssrc_, payloadtype_);

    //update sequence after rtx demux
    update_seq(pkt->get_seq());

    if (media_type_ == "video" && nack_handle_) {
        log_debugf("++++ receive video rtx pkt ssrc:%u, seq:%d", pkt->get_ssrc(), pkt->get_seq());
        nack_handle_->update_nacklist(pkt);
    }
    return;
}

void rtp_recv_stream::update_rtt(int64_t rtt) {
    rtt_ = rtt;
    if (nack_handle_) {
        nack_handle_->update_rtt(rtt);
    }
}

void rtp_recv_stream::generate_nacklist(const std::vector<uint16_t>& seq_vec) {
    rtcp_fb_nack* nack_pkt = new rtcp_fb_nack(0, ssrc_);
    nack_pkt->insert_seq_list(seq_vec);

    cb_->stream_send_rtcp(nack_pkt->get_data(), nack_pkt->get_len());

    delete nack_pkt;
}

void rtp_recv_stream::get_statics(json& json_data) {
    size_t fps;
    size_t speed = recv_statics_.bytes_per_second((int64_t)now_millisec(), fps);

    json_data["bps"]       = speed * 8;
    json_data["fps"]       = fps;
    json_data["count"]     = recv_statics_.get_count();
    json_data["bytes"]     = recv_statics_.get_bytes();
    json_data["jitter"]    = jitter_;
    json_data["lostper"]   = lost_percent_;
    json_data["losttotal"] = total_lost_;
    return;
}