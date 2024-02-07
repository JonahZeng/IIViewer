# About 
----
this repo is designed for open and view ISP intermediate image. we support these format below:
- jpg
- pnm
- raw
- bmp
- png

## how to use
use this software is very easy, just double click this application icon from your files explore, you can see this if there is no any unexpected error:
![main-ui](./doc/image/main-ui.png)
like its tip, drag any supported format image to dash rectangle，it will display the image context.
that's all.

## how to build from source code
### windows
- install Qt5 or Qt6, cmake, MSVC v143
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