# 流媒体服务器性能测试
## 1. RTMP 性能测试
### 1.1 性能测试指标
- 并发连接数(虚拟用户数)
- 丢包率
- 平均下载速度
- 视频首包用时
- 视频首帧用时
- 平均帧率
### 1.2 Linux网络分析工具
- iftop
- nethogs
### 1.3 FFmpeg相关命令

- 循环推流

  ```shell
  ffmpeg -re -stream_loop -1 -i test.flv -c copy -f flv rtmp://127.0.0.1/live/livestream
  ```

- 

[Capturing your Desktop / Screen Recording](https://trac.ffmpeg.org/wiki/Capture/Desktop)

[FFmpeg捕获音频流](https://trac.ffmpeg.org/wiki/Capture/PulseAudio)



### 1.4 其他工具


### References
[1] [16个有用的带宽监控工具，用于分析Linux中的网络使用情况](https://cn.linux-console.net/?p=192)
[2] [如何对流媒体RTMP/HLS协议进行性能测试](http://bbs.51testing.com/thread-1205842-1-1.html)
[3] [rtmp-bench](https://www.npmjs.com/package/rtmp-bench)

[4] [[SOLVED] ffmpeg fails, Cannot open display :0.0, error 1](https://www.linuxquestions.org/questions/linux-desktop-74/ffmpeg-fails-cannot-open-display-0-0-error-1-a-4175613512/)

[5] [音视频开发（5）FFmpeg流媒体](http://iherr.cn/2016/12/27/%E9%9F%B3%E8%A7%86%E9%A2%91%E5%BC%80%E5%8F%91%EF%BC%885%EF%BC%89FFmpeg%E6%B5%81%E5%AA%92%E4%BD%93/)

