# RTMP media_server Performance Test

### 1、测试环境

- OS：Ubuntu20.04LTS 64bit，Liux kernel 5.13.0
- CPU：Intel Core i7-8550U @ 1.8GHZ x 8（4 physic cores and 8 logic cores）
- MEM：16GB
- NET：LAN, Unlimited bandwidth

### 2、延迟测试

#### 2.1 测试工具

- 计时器

- FFmpeg
- obs
- ffplay

#### 2.2 延迟测试方法

在PC上开启一个精确到毫秒的计时器，再通过工具捕获桌面，将视频编码推流 RTMP 流媒体服务器，同时再开启一个播放器。通过屏幕截图将源视频和播放视频包含在同一张图片内，计算时间差值，这样便可做到准确的延时统计。

#### 2.3 延迟测试结果

我使用如下 FFmpeg 命令捕获屏幕并通`rtmp://192.168.3.148/live/livestream`地址发送到 RTMP 流媒体服务器

```shell
ffmpeg -f x11grab -framerate 25 -video_size 700x610 -i $DISPLAY -vcodec libx264 -preset ultrafast -f flv rtmp://192.168.3.148/live/livestream
```

然后在本地使用 ffplay 命令拉取 RTMP 流

```shell
ffplay -i rtmp://192.168.3.148/live/livestream
```

测试结果如下图所示，延迟为`11.16-5.56=5.6`。测得延迟为5.6秒，延迟超过5秒，显然不符合预期。

![latency-test-1](https://cdn.jsdelivr.net/gh/Kevinnan-teen/CDN/image-20220322221233168.png)

由于这是在局域网内进行传输，网络应该不存在瓶颈。那问题应该在本地 FFmpeg 编码推流耗时或者是 RTMP 流媒体服务器性能太差。

通过分析本地 FFmpeg 编码推流的输出日志（如下图所示）可以发现，最开始捕获屏幕进行编码时速度很慢，从 0.234 开始逐渐增加到 0.99x。这就造成使用 ffplay 拉流时视频内的时间和源视频差距过大，超过5秒。

![ffmpeg-publish](https://cdn.jsdelivr.net/gh/Kevinnan-teen/CDN/image-20220323005313468.png)

既然直接使用 FFmpeg 命令进行编码推流会造成延迟，那么换一个成熟的推流工具应该解决这个问题。于是我选择 obs 作为编码推流软件，设置推流和视频输出参数（如下图所示），然后进行推流。

![obs-1](https://cdn.jsdelivr.net/gh/Kevinnan-teen/CDN/image-20220323005941964.png)

![obs-2](https://cdn.jsdelivr.net/gh/Kevinnan-teen/CDN/image-20220323010004186.png)

测试结果如下图所示，延迟为`28.91-26.54=2.37`。测得延迟为 2.37 秒，延迟在 2 秒左右，符合 RTMP 的延迟要求。

![latency-test-4](https://cdn.jsdelivr.net/gh/Kevinnan-teen/CDN/image-20220322230925439.png)

### 3、并发连接数测试

#### 3.1 测试工具

- FFmpeg
- [rtmp-bench](https://github.com/illuspas/rtmp-bench)压力测试工具
- Wireshark 抓包

#### 3.2 并发测试方法

对于流媒体服务器来说，它不仅要接受多路的推流输入，还要承载多路的拉流输出。因此能承载多少并发连接数是衡量流媒体服务器性能的重要指标。

由于本次测试推流端、拉流端、服务器都是在本机进行测试，因为我们不考虑网络带宽的限制。只是测量在本机硬件条件下，不同的并发数对 CPU/内存的占用情况。

#### 3.3 测试过程

##### 3.3.1 测试最大推流并发数

使用 rtmp-bench 创建 100 个 RTMP 推流连接

```shell
rtmp-bench -f capture-90s.flv -r rtmp://192.168.3.148/live/stream -c 100
```

如下图所示，此时 CPU 占用率约为 27%，内存占用率约为 20%。

![image-20220323022627458](https://cdn.jsdelivr.net/gh/Kevinnan-teen/CDN/image-20220323022627458.png)

使用 rtmp-bench 创建 200 个 RTMP 推流连接。如下图所示，此时 CPU 占用率约为 42%，内存使用率约为 20%。

![image-20220323023205585](https://cdn.jsdelivr.net/gh/Kevinnan-teen/CDN/image-20220323023205585.png)

使用 rtmp-bench 创建 500 个 RTMP 推流连接。如下图所示，此时 CPU 占用率约为 65%，内存使用率约为 32%。

![](https://cdn.jsdelivr.net/gh/Kevinnan-teen/CDN/image-20220323023513185.png)

使用 rtmp-bench 创建 1000 个 RTMP 推流连接，同时创建 100 个拉流连接。如下图所示，此时 CPU 占用率约为 100%，内存使用率约为 56%。

![](https://cdn.jsdelivr.net/gh/Kevinnan-teen/CDN/image-20220323024717224.png)

##### 3.3.2 测试最大拉流连接

使用 rtmp-bench 创建 50 个 RTMP 推流连接，同时创建 500 个拉流连接。如下图所示，此时 CPU 占用率约为 17%，内存使用率约为 32%。

![image-20220323024239652](https://cdn.jsdelivr.net/gh/Kevinnan-teen/CDN/image-20220323024239652.png)

使用 rtmp-bench 创建 50 个 RTMP 推流连接，同时创建 1000 个拉流连接。如下图所示，此时 CPU 占用率约为 100%，内存使用率约为 32%。

![](https://cdn.jsdelivr.net/gh/Kevinnan-teen/CDN/image-20220323024345569.png)

使用 rtmp-bench 创建 100 个 RTMP 推流连接，同时创建 1000 个拉流连接。如下图所示，此时 CPU 占用率约为 100%，内存使用率约为 32%。

![](https://cdn.jsdelivr.net/gh/Kevinnan-teen/CDN/image-20220323024448123.png)

##### 3.3.2 Wireshark 抓包

使用 rtmp-bench 创建 100 个 RTMP 推流连接，同时创建 500 个拉流连接。如下图所示，Port A 为推流和拉流端口，Port B 为 RTMP 流媒体服务器端口。Bit/s A->B 速率约为1200k，代表推流码率约为1200kbps。

![](https://cdn.jsdelivr.net/gh/Kevinnan-teen/CDN/Wireshark · Conversations · Loopback: lo_001.png)

#### 3.4 测试结果

通过测试不同的 RTMP 推流、拉流连接数，可以得出结论，RTMP 流媒体服务器支持的推流最高并发数约为1000，拉流最高并发数约为1000。

另外，由于本次测试是在本机进行，所以忽略了带宽的影响。而实际情况下，以最常见的千兆带宽为例，假设推流视频码率为2000kbps，那么最多可支持500路流的并发。本次测试只是测量在现有 CPU 和内存条件下可以支持的最高并发数，并对服务器进行压力测试，在实际生产环境中此测试结果只有有限参考价值。

在实际的应用场景中，客户端一般不会直连流媒体服务器，而是部署在全国各地的 CDN 服务器先从流媒体服务器请求流，然后由 CDN 服务器进行流的分发。因此直连流媒体服务器的只是一些 CDN 服务器，只有有限的连接数。

### 4、其他未测试的性能指标

- 捕获屏幕推流过程中，屏幕中画面快速变化对播放端的帧率、流畅性、卡顿可能造成的影响
- 秒开（直播画面第一次展示与刚开始推流的时间间隔）
- 网络信号强弱对播放端画面、码率、帧率影响
- 丢包率

> 本项目的核心指标是优化流媒体协议的延迟。也即 WebRTC 相较于 RTMP 在时延方面的改进。由于以上指标都是在复杂网络环境上才可能出现的问题，因此不在主要考虑范围内，暂时不进行测试。
>
> 之后的主要精力放在 WebRTC 服务器的实现上。
>
> WebRTC 相较于 RTMP 的核心优势是什么？WebRTC 的媒体传输部分基于 UDP，而 RTMP 基于 TCP。TCP 要做网络质量控制需要花费大量开销，而 UDP 不需要。WebRTC 只是基于 UDP 在应用层做了一些诸如丢包重传、网络抖动的处理，开销相对较小。

