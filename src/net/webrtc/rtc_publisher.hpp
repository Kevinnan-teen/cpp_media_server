#ifndef RTC_PUBLISH_HPP
#define RTC_PUBLISH_HPP
#include "rtc_media_info.hpp"
#include "net/rtprtcp/rtp_packet.hpp"
#include "net/rtprtcp/rtcp_sr.hpp"
#include "net/rtprtcp/rtcp_xr_dlrr.hpp"
#include "net/rtprtcp/rtcp_xr_rrt.hpp"
#include "utils/timer.hpp"
#include "utils/timeex.hpp"
#include "rtp_recv_stream.hpp"
#include "rtc_stream_pub.hpp"
#include "jitterbuffer_pub.hpp"
#include "jitterbuffer.hpp"
#include "pack_handle_pub.hpp"
#include "data_buffer.hpp"
#include "json.hpp"

#include <vector>
#include <queue>
#include <stdint.h>
#include <stddef.h>
#include <string>

using json = nlohmann::json;

extern int get_publisher_statics(const std::string& roomId, const std::string& uid, json& data_json);

class rtc_base_session;
class room_callback_interface;

class rtc_publisher : public timer_interface, public rtc_stream_callback, public jitterbuffer_callbackI, public pack_callbackI
{
public:
    rtc_publisher(const std::string& roomId, const std::string& uid,
            room_callback_interface* room, rtc_base_session* session, const MEDIA_RTC_INFO& media_info);
    virtual ~rtc_publisher();

public:
    friend int get_publisher_statics(const std::string& roomId, const std::string& uid, json& data_json);

public:
    void on_handle_rtppacket(rtp_packet* pkt);
    void on_handle_rtcp_sr(rtcp_sr_packet* sr_pkt);

public:
    MEDIA_RTC_INFO get_media_info() {return media_info_;}
    std::string get_publisher_id() {return pid_;}
    std::string get_media_type();
    uint32_t get_rtp_ssrc() {return rtp_ssrc_;}
    uint32_t get_rtx_ssrc() {return rtx_ssrc_;}
    int get_mid() {return media_info_.mid;}
    int get_abstime_id() {return abs_time_extension_id_;}
    int get_clockrate();
    uint8_t get_rtp_payloadtype();
    uint8_t get_rtx_payloadtype();
    bool has_rtx();
    void set_stream_type(const std::string& stream_type) { stream_type_ = stream_type; }
    std::string get_stream_type() { return stream_type_; }
    MEDIA_CODEC_TYPE get_codec_type() { return codec_type_; }
    void get_statics(json& json_data);

public:
    void request_keyframe(uint32_t media_ssrc);
    void get_xr_rrt(xr_rrt& rrt, int64_t now_ms);
    void handle_xr_dlrr(xr_dlrr_data* dlrr_block);

public://implement timer_interface
    virtual void on_timer() override;

public://implement rtc_stream_callback
    virtual void stream_send_rtcp(uint8_t* data, size_t len) override;
    virtual void stream_send_rtp(uint8_t* data, size_t len) override;

public://implement jitterbuffer_pub
    virtual void rtp_packet_reset(std::shared_ptr<rtp_packet_info> pkt_ptr) override;
    virtual void rtp_packet_output(std::shared_ptr<rtp_packet_info> pkt_ptr) override;

public://implement 
    virtual void pack_handle_reset(std::shared_ptr<rtp_packet_info> pkt_ptr) override;
    virtual void media_packet_output(std::shared_ptr<MEDIA_PACKET> pkt_ptr) override;

private:
    void set_rtmp_info(std::shared_ptr<MEDIA_PACKET> pkt_ptr);
    
private:
    std::string roomId_;
    std::string uid_;
    room_callback_interface* room_ = nullptr;
    rtc_base_session* session_ = nullptr;

private:
    std::string pid_;
    MEDIA_RTC_INFO media_info_;
    std::string media_type_str_;
    MEDIA_PKT_TYPE media_type_ = MEDIA_UNKOWN_TYPE;
    MEDIA_CODEC_TYPE codec_type_ = MEDIA_CODEC_UNKOWN;
    std::string stream_type_   = "camera";
    uint32_t rtp_ssrc_         = 0;
    uint32_t rtx_ssrc_         = 0;
    int channel_               = 2;
    int clock_rate_            = 0;
    uint8_t payloadtype_       = 0;
    uint8_t rtx_payloadtype_   = 0;
    bool has_rtx_              = false;
    int mid_extension_id_      = 0;
    int abs_time_extension_id_ = 0;

private:
    rtp_recv_stream* rtp_handler_  = nullptr;
    jitterbuffer jb_handler_;
    pack_handle_base* pack_handle_ = nullptr;
    data_buffer sps_data_;
    data_buffer pps_data_;
    std::queue<std::shared_ptr<MEDIA_PACKET>> flv_queue_;
    bool first_flv_audio_ = true;

private:
    int64_t last_rrt_ = 0;
    float   rtt_      = 0;

private:
    int64_t last_keyrequest_ts_ = 0;
};

#endif
