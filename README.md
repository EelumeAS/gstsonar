# gstsonar

GStreamer elements for processing sonar data from [Norbit WBMS FLS](https://norbit.com/subsea/products/).

## Prerequisites

[GStreamer](https://gstreamer.freedesktop.org/download/)

SDL2

OpenGL, GLEW

## Build

```
mkdir build
cd build
cmake ..
make
```

## Install
```
make install
```


## Example launch lines

Note that you need to set `GST_PLUGIN_PATH=$(pwd)` if running from build directory without installing.
Ie. `GST_PLUGIN_PATH=$(pwd) gst-launch-1.0 ...`.
Extra debugging info can be printed with: `GST_DEBUG=2,sonarparse:9` etc.

View sonar data from file:

```
gst-launch-1.0 filesrc location=../samples/in.sbd ! sonarparse ! sonarsink zoom=5
```

View sonar data from tcp:

```
gst-launch-1.0 tcpclientsrc host=192.168.3.58 port=2211 ! sonarparse ! sonarsink zoom=12
```


View sonar data from file as normal image:
```
gst-launch-1.0 filesrc location=../samples/in.sbd ! sonarparse ! sonarconvert ! videoconvert ! autovideosink
```

Parse both sonar and telemetry data:
```
gst-launch-1.0 filesrc location=../samples/in.sbd ! tee name=t ! queue ! sonarparse ! sonarsink t. ! queue ! nmeaparse ! fakesink
```

## Settings on norbit sonar

Connect to control interface:
```
nc 192.168.3.58 2209
```

Commands:
```
set_power 1
set_flip 1
set_resolution 640 512
set_range 1 25
```
