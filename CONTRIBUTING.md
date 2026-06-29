# Contributing to IIViewer

Thanks for your interest in contributing! This guide covers everything you need to get started.

## Development Setup

### Prerequisites

- CMake >= 3.20
- C++17 compatible compiler (MSVC 2022, GCC 11+, or AppleClang)
- Qt6 (6.8+) with modules: Widgets, DataVisualization, Network, Charts, LinguistTools
- OpenSSL, libheif, libde265 (bundled in `thirdparty/`)

> For Qt5 support, see the [qt5 branch](https://github.com/JonahZeng/IIViewer/tree/qt5).

### Build

**Windows (MSVC)**
```powershell
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=<Qt6Path> -Wno-dev -S ..
cmake --build . --config Release
```

**Linux (Ubuntu)**
```bash
sudo apt install build-essential libssl-dev libgl1-mesa-dev
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

**macOS (Apple Silicon)**
```bash
brew install qt
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
cmake --build build -j$(sysctl -n hw.ncpu)
```

### Build third-party libraries (Windows)

1. Build and install OpenSSL. Copy its install dir to `thirdparty/` directory.

2. Build libde265:
   ```bash
   wget https://github.com/strukturag/libde265/releases/download/v1.0.16/libde265-1.0.16.tar.gz
   tar -zxf libde265-1.0.16.tar.gz
   cd libde265-1.0.16 && mkdir build && cd build
   cmake .. -G "Visual Studio 17 2022" -A x64 \
       -DENABLE_DECODER=OFF -DENABLE_ENCODER=OFF -DENABLE_SDL=OFF \
       -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release \
       -DCMAKE_INSTALL_PREFIX=<repo>/thirdparty/libde265/msvc
   cmake --build . --config Release
   cmake --install . --config Release
   ```

3. Build libheif:
   ```bash
   wget https://github.com/strukturag/libheif/releases/download/v1.21.2/libheif-1.21.2.tar.gz
   tar -zxf libheif-1.21.2.tar.gz
   cd libheif-1.21.2 && mkdir build && cd build
   cmake .. -G "Visual Studio 17 2022" -A x64 \
       -DWITH_LIBDE265=ON -DBUILD_SHARED_LIBS=OFF \
       -DLIBDE265_INCLUDE_DIR=<repo>/thirdparty/libde265/msvc/include \
       -DLIBDE265_LIBRARY=<repo>/thirdparty/libde265/msvc/lib/libde265.lib \
       -DCMAKE_INSTALL_PREFIX=<repo>/thirdparty/libheif/msvc
   cmake --build . --config Release
   cmake --install . --config Release
   ```

### Package

**Windows:**
```powershell
cpack -C Release -G ZIP   # or -G WIX (requires WiX Toolset)
```

**Linux (DEB):**
```bash
cd build && cpack -G DEB -C Release
```

## Code Style

### File Organization

- Source files: `src/`
- Header files: `inc/`
- UI files (Qt Designer): `inc/*.ui`
- Translation files: `translations/`

### Naming Conventions

- **Classes**: PascalCase (`IIViewer`, `ImageWidget`, `AppSettings`)
- **Functions/Methods**: camelCase (`loadFile`, `setPixmap`, `zoomIn`)
- **Variables**: camelCase
- **Constants**: UPPER_SNAKE_CASE for macros, camelCase for constexpr
- **Enums**: PascalCase for names and values (`YuvType::YUV420_NV12`)

### Include Order

1. Project-specific headers (quoted): `"IIViewer.h"`
2. Qt headers (angled): `<QMainWindow>`, `<QImage>`
3. Standard library (angled): `<array>`, `<memory>`

```cpp
#include "config.h"
#include "IIViewer.h"
#include <QApplication>
#include <QMessageBox>
#include <array>
```

### Header Guards

Use `#pragma once` or `#ifndef`/`#define`/`#endif`:

```cpp
#ifndef IIVIEWER_H
#define IIVIEWER_H
// ...
#endif // IIVIEWER_H
```

### Classes

- Mark leaf classes as `final`: `class ImageWidget final : public QWidget`
- Use `explicit` on single-argument constructors
- Use `override` on virtual method overrides
- Delete copy constructor/assignment when appropriate

### Member Initialization

Use member initializer lists and in-class initialization:

```cpp
AppSettings::AppSettings()
    : yuvType(YuvType::YUV444_INTERLEAVE),
      yuvBitDepth(8),
      rawCompact(false)
{
}
```

### Constants

Prefer `constexpr` for compile-time constants:

```cpp
constexpr int LEFT_IMG_WIDGET = 0;
constexpr int BIT8 = 8;
```

### Lint Warnings

Suppress intentional violations with NOLINT comments:

```cpp
int value = 8; // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
```

### Error Handling

- Use Qt's return-value based error handling pattern
- Check file existence and validity before operations
- Use `QMessageBox` for user-facing errors

### Qt Patterns

- Use `Q_OBJECT` macro in classes with signals/slots
- Use `emit` keyword when emitting signals
- Prefer Qt containers (`QString`, `QList`, `QMap`) in Qt code
- Use `Q_UNUSED` or anonymous variable names for unused parameters

### Platform-Specific Code

Use Qt's platform macros:

```cpp
#ifdef Q_OS_WINDOWS
    // Windows-specific code
#endif
#ifdef Q_OS_MACOS
    // macOS-specific code
#endif
```

### Memory Management

- Qt parent-child system handles widget memory
- Use smart pointers for non-Qt managed memory
- Raw pointers are acceptable when ownership is clear (Qt pattern)

### Comments

- Comments may be in Chinese or English
- Document complex algorithms and non-obvious logic
- Use `//` for single-line, `/* */` for multi-line

## Static Analysis

Clang-tidy is configured via `.clang-tidy`:

```powershell
# Generate compile_commands.json first
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=<Qt6Path> -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S ..
clang-tidy -p build src/*.cpp
```

Enabled checks: `bugprone-*`, `performance-*`, `readability-*`, `modernize-*`, `cppcoreguidelines-*`, `clang-analyzer-*`, `portability-*`

## Translation

1. Run `lupdate` to scan source and generate/update `.ts` files:
   ```bash
   lupdate ./src/*.cpp ./inc/*.h -ts ./translations/IIViewer_zh.ts
   ```
2. Use Qt Linguist to translate the `.ts` file, then release it to a `.qm` file
3. The app loads the appropriate `.qm` file at startup based on the user's locale

## Testing

There are no automated tests configured. Please test manually by running the application and verifying image loading/display functionality for the formats you change.

## Pull Request Process

1. Fork the repository and create a branch from `main`
2. Make your changes, following the code style above
3. Run clang-tidy on changed files if possible
4. Manually test the affected functionality
5. Submit a pull request with a clear description of the change and motivation