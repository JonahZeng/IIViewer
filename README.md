<h1 align="center">IIViewer</h1>
<p align="center">
    <strong>A professional tool which designed for image signal process developer</strong>
</p>
<p align="center">
    <img src="./icon/64.png" alt="IIViewer icon" width="64"/>
</p>

<p align="center">
    <img src="https://img.shields.io/github/actions/workflow/status/JonahZeng/IIViewer/cmake-windows-platform.yml" alt="GitHub Actions Workflow Status"/>
</p>

## About
this repo is designed for open and view ISP intermediate image. we support these format below:
- jpg
- bmp
- png
- pnm(8/10/12/14/16 bit)
- raw(8/10/12/14/16/18/20/22/24 bit)
- yuv(8/10/12 444-interleave4 444-plannar 422-UYVY 422-YUYV 420-NV12 420-NV21 420P-YU12 420P-YV12 400)

## Usage
download it from [release page](https://github.com/JonahZeng/IIViewer/releases), start this app on your PC, you can see this if there is no any unexpect error:
![main-ui](./doc/image/main-ui.png)
as its tips, drag any supported format image to dash rectangle，it will display the image context. when you zoom in to 96X by scroll your mouse wheel, you will see every pixel's real value.
that's all.

## Build from source code
1. build and install OpenSSL, if you works on Windows platform, here is offical [repo](https://github.com/openssl/openssl) and build [guide](https://github.com/openssl/openssl/blob/master/NOTES-WINDOWS.md), once you build and install it successed, copy its install dir to this git repo **thirdparty** directory. if you works on Linux, install OpenSSL with your software package manager like atp, yum, dnf.

2. install Qt5 with necessary module:
    - Widgets
    - Gui
    - Core 
    - DataVisualization(**commercial license**)
    - Network(**enable ssl**)

if you install Qt by build from soure, here is my configuration(MinGW64 13.2.0, Qt5.15.15) for reference:
```bash
.\configure -prefix {qt_source\qtbase} \
-opensource -confirm-license \
-openssl-runtime \
-I {openssl3 header direcotry} \
-L {openssl3 library directory} \
-nomake tests \
-nomake examples \
-opengl desktop \
-release \
-skip webview \
-skip webengine \
-skip webglplugin \
-skip webchannel 
```
MSVC build Qt5.15.16:
```bat
configure -prefix %CD%\qtbase -opensource -confirm-license -nomake tests -nomake examples -release -skip webview -skip webengine -skip webglplugin -skip webchannel -openssl-runtime -I {openssl3 header direcotry} -L {openssl3 library directory} -make-tool jom -platform win32-msvc
```
### windows(amd64)
I have test it both on windows10 with mingw64 13.2.0 and windows11 with MSVC v143, Qt version >= 5.15.2.
follow these steps:
- install Qt5 or Qt6, cmake, MSVC or MinGW64 (should support **C++11 at least**)
- git clone this repo
- mk dir build, and run cmake in this directory
```bat
git clone https://github.com/JonahZeng/IIViewer.git
cd IIViewer
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=YOUR_QT_INSTALL_DIR
cmake --build . --config Release
```
don't forget copy `libssl-3-x64.dll` and `libcrypto-3-x64.dll` to `bin/[Release|Debug]` when cmake build it finished, as you see, we enable openssl with option `-openssl-runtime` when configure/build Qt.

### linux(amd64)
only tested it on ubuntu 20.04(cmake 3.16, Qt5.12.8, gcc 9.4.0), here are the steps:
```bash
sudo apt install build-essential qt5-default libqt5datavisualization5-dev cmake libssl-dev
git clone https://github.com/JonahZeng/IIViewer.git
cd IIViewer
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j 4
```

### macos
I didn't have any mac device, but I'm sure compile this project on macos would not be diffcult since this project use cross-platform build tool(cmake).
