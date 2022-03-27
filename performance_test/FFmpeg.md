### 1、Linux FFmpeg 

#### 1.1 v4l2 摄像头录制

查看 v4l2 摄像头所支持的色彩格式及分辨率：

```shell
ffmpeg -hide_banner -f v4l2 -list_formats all -i /dev/video0
```

把这个摄像头采集为视频文件：

```shell
ffmpeg -hide_banner -s 1920x1080 -i /dev/video0 output.flv
```

#### 1.2 x11grab 屏幕录制

FFmpeg通过 x11grab 录制屏幕时，输入设备的设备名规则如下：

```tex
[主机名:]显示编号id.屏幕编号id+起始x轴，起始y轴
```

使用`echo $DISPLAY`查看显示编号id

```shell
ffmpeg -f x11grab -framerate 25 -video_size 1920x1080 -i $DISPLAY out.flv
```

#### 1.3 Linux 录制音视频

```shell
ffmpeg -f x11grab -framerate 25 -video_size 1920x1080 -i $DISPLAY -f pulse -i alsa_output.usb-USB_Microphone_Maono_Fairy_2020_11_21-00.iec958-stereo.monitor -ac 2 -vcodec libx264 -preset medium -b:v 2000k -acodec aac capture.flv
```

```shell
fmpeg -f x11grab -framerate 25 -video_size 1920x1080 -i $DISPLAY -f pulse -i alsa_output.usb-USB_Microphone_Maono_Fairy_2020_11_21-00.iec958-stereo.monitor -ac 2 -vcodec libx264 -preset ultrafast -b:v 2000k -acodec aac -f flv rtmp://192.168.3.148/live/livestream
```

#### 1.4 RTMP 推流

推本地流

```shell
ffmpeg -re -stream_loop -1 -i test.flv -c copy -f flv rtmp://192.168.3.148/live/livestream
```

推实时捕获流

```shell
ffmpeg -f x11grab -framerate 25 -video_size 1280x720 -i $DISPLAY -vcodec libx264 -preset ultrafast -b:v 2000k -f flv rtmp://192.168.3.148/live/livestream
```
