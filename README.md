# gstsonar

GStreamer elements for processing sonar data from [Norbit WBMS STX and FLS sonars](https://norbit.com/subsea/products/).
Easily extentible to other sonars.

Gstsonar is licensed under the [LGPL v2.1 license](LICENSE)

[https://github.com/EelumeAS/gstsonar](https://github.com/EelumeAS/gstsonar)

![sonar image](doc/images/sonarsink-image.png)

## Prerequisites
Install the following libraries that are prerequisites for building gstsonar.

### Ubuntu:

```
$ sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-good libsdl2-dev libglew-dev libeigen3-dev
```

### Windows:

* [EIGEN3](https://eigen.tuxfamily.org/index.php?title=Main_Page)
* [GStreamer](https://gstreamer.freedesktop.org/download/)
* [SDL2](https://buildbot.libsdl.org/sdl-builds/sdl-visualstudio-amd64/)
* [GLEW](https://sourceforge.net/projects/glew/)
* OpenGL (Preinstalled on Windows)

For Windows, you must also manually add the dll file directories of SDL2 and GLEW to the %PATH% environment variable.
Also, you must edit the paths to the libraries in CMakeLists.txt .
Not sure how to avoid all the manual configuration in Windows ¯\\_(ツ)_/¯.

## Check-out GIT LFS files
The sample files in ```samples/``` are stored using GIT LFS. Files stored with LFS are normally checked out and updated as normally, but if
the files are not updated automatically, they can be updated using the commands
```
$ sudo apt-get install git-lfs
$ git lfs pull
```

## Build

```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Install
```
$ make install
```

## GStreamer Plug-ins

| Plugin | Description | 
| --- | --- |
| `sonarparse` | Parses sonar data from both SBD frames and TCP frames from a Norbit sonar. |
| `sonarsink`  | Takes a stream of either `sonar/bathymetry` or `sonar/multibeam` and shows the data. Press "ESC" to exit, press "SPACE" to pause. |
| `sonarmux`   | Sonar mux that combines sonar data and telemetry data. |
| `nmeaparse`  | Parses NMEA data from SBD frames or TCP frames.| 
| `eelnmeadec` | Decodes telemetry/navigation data in Eelume navigation format. |

## Example launch lines

Note that you need to set `GST_PLUGIN_PATH=.` if running from build directory without installing the plugin.
_Ie._ `GST_PLUGIN_PATH=. gst-launch-1.0 ...`.
Extra debugging info can be printed with: `GST_DEBUG=2,sonarparse:9` etc.

View sonar data from file:
```
$ gst-launch-1.0 filesrc location=../samples/in.sbd ! sonarparse ! sonarsink zoom=1.2
```
or (full command line for running plugin ```libgstsonar.so``` located in ```.``` folder):
```
$ GST_PLUGIN_PATH=. GST_DEBUG=4 gst-launch-1.0 filesrc location=../samples/in.sbd ! sonarparse ! sonarsink zoom=1.2
```

View sonar data (multibeam/FLS or bathymetry) from tcp:
```
$ gst-launch-1.0 tcpclientsrc host=192.168.6.58 port=2211 ! sonarparse ! sonarsink zoom=4
$ gst-launch-1.0 tcpclientsrc host=192.168.6.121 port=2210 ! sonarparse ! sonarsink zoom=0.04
```

Parse both sonar and telemetry data:
```
$ SBD=../samples/in.sbd && GST_PLUGIN_PATH=. GST_DEBUG=2 gst-launch-1.0 filesrc location=$SBD ! sonarparse ! sonarmux name=mux ! sonarsink filesrc location=$SBD ! nmeaparse ! eelnmeadec ! mux.
# (tee apparently doesn't go well with filesrc: gst-launch-1.0 filesrc location=../samples/in.sbd ! tee name=t ! queue ! sonarparse ! sonarsink t. ! queue ! nmeaparse ! eelnmeadec ! fakesink)
```

Parse both sonar and telemetry data from tcp:
```
GST_PLUGIN_PATH=. gst-launch-1.0 tcpclientsrc host=192.168.6.58 port=2211 ! sonarparse ! sonarmux name=mux ! sonarsink tcpclientsrc host=192.168.6.100 port=11000 ! nmeaparse ! eelnmeadec ! mux.
```

# Libraries used in Gstsonar
The following tables lists the open-source projects used in Gstsonar, licensed under open-source licenses:

| Library | License |
| --- | --- |
| [EIGEN3](https://eigen.tuxfamily.org/index.php?title=Main_Page) | MPL2 |
| [GLEW](https://glew.sourceforge.net/) |  The source code is licensed under the Modified BSD License, the Mesa 3-D License (MIT) and the Khronos License (MIT) |
| [Gstreamer](https://gstreamer.freedesktop.org/) | LGPL |
| [SDL](https://www.libsdl.org/) | [zlib license](https://www.libsdl.org/license.php) |
| [wbms_georef](https://github.com/magnuan/wbms_georef) | MIT License |
