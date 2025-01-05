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
### windows(AMD64)
#### prepare
1. install cmake
2. install MSVC or MinGW64 (should support **C++17 at least**)
3. build and install OpenSSL, here is offical [repo](https://github.com/openssl/openssl) and build [guide](https://github.com/openssl/openssl/blob/master/NOTES-WINDOWS.md), once you build and install it successed, copy its install dir to this git repo **thirdparty** directory. 

4. install Qt5 with necessary module:
    - Widgets
    - Gui
    - Core 
    - DataVisualization
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
#### build
I have test it both on windows10 with mingw64 13.2.0 and windows11 with MSVC v143, Qt version >= 5.15.2.
follow these steps:
```bat
git clone https://github.com/JonahZeng/IIViewer.git
cd IIViewer
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=YOUR_QT_INSTALL_DIR
cmake --build . --config Release
```

### linux(amd64)
#### prepare
install cmake, gcc, openssl and Qt5 
```bash
sudo apt install cmake build-essential libssl-dev qt5-default libqt5datavisualization5-dev qttools5-dev-tools
```
#### build
these build steps had been tested on ubuntu 20.04(cmake 3.16, Qt5.12.8, gcc 9.4.0)
```bash
git clone https://github.com/JonahZeng/IIViewer.git
cd IIViewer
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j 4
```
#### pack to Deb or AppImage
for deb:
```bash
cpack -G DEB -C Release
```
for AppImage, you should download [linuxdeployqt](https://github.com/probonopd/linuxdeployqt/releases), and create one .desktop and .png file in `bin/Release` directory, here are the desktop file context:
```text
[Desktop Entry]
Type=Application
Name=IIViewer
Exec=AppRun %F
Icon=default
Comment=isp develop image viewer
Terminal=false
Categories=Development;
```
download and execute linuxdeployqt:
```bash
wget https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod a+x ./linuxdeployqt-continuous-x86_64.AppImage
./linuxdeployqt-continuous-x86_64.AppImage {your-build-IIViewer-bin-file-path} -verbose=2 -bundle-non-qt-libs -appimage
```

### macos
I didn't have any mac device, but I'm sure compile this project on macos would not be diffcult since this project use cross-platform build tool(cmake).
