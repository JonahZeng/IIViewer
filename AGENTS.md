# IIViewer

A cross-platform image viewer designed for ISP (Image Signal Process) developers. Supports various raw image formats including RAW, MIPI-RAW, RGBIR-RAW, YUV, PGM, PNM, HEIF, and standard formats (JPG, BMP, PNG, TIFF).

## Build Commands

### Prerequisites
- CMake >= 3.20
- C++17 compatible compiler (MSVC 2022, GCC 11+, or AppleClang)
- Qt5 (5.15.2+) or Qt6 with modules: Widgets, DataVisualization, Network, Charts, LinguistTools
- OpenSSL, libheif, libde265 (bundled in thirdparty/)

### Windows (MSVC)
```powershell
# Configure (work directory: {ROOT}/build)
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=D:/software/qt51516/qtbase -Wno-dev -S ..

# Build Release
cmake --build . --config Release

# Build Debug (includes Address Sanitizer)
cmake --build . --config Debug

# Install
cmake --install . --config Release

# Package (NSIS installer)
cpack -C Release -G NSIS
```

### Linux (Ubuntu)
```bash
# Install dependencies
sudo apt install build-essential libssl-dev qtbase5-dev libqt5datavisualization5-dev \
    qtbase5-dev-tools libqt5charts5-dev qttools5-dev-tools qttools5-dev

# Configure and build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Package as DEB
cd build && cpack -G DEB -C Release
```

### macOS (Apple Silicon)
```bash
# Install Qt5 via Homebrew
brew install qt@5

# Configure and build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5 \
    -DLRELEASE_EXECUTABLE=/opt/homebrew/opt/qt@5/bin/lrelease

cmake --build build -j$(sysctl -n hw.ncpu)
```

## Testing

No automated tests are configured in this project. Testing is performed manually by running the application and verifying image loading/display functionality.

## Lint/Static Analysis

Clang-tidy is configured via `.clang-tidy`. Run analysis with:
```powershell
# Generate compile_commands.json first
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=<QtPath> -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S ..

# Run clang-tidy (requires compile_commands.json)
clang-tidy -p build src/*.cpp
```

Enabled checks include: bugprone-*, performance-*, readability-*, modernize-*, cppcoreguidelines-*, clang-analyzer-*, portability-*

## Code Style Guidelines

### File Organization
- Source files: `src/`
- Header files: `inc/`
- UI files (Qt Designer): `inc/*.ui`
- Translation files: `translations/`

### Naming Conventions
- **Classes**: PascalCase (e.g., `IIPviewer`, `ImageWidget`, `AppSettings`)
- **Functions/Methods**: camelCase (e.g., `loadFile`, `setPixmap`, `zoomIn`)
- **Variables**: camelCase for locals, snake_case with prefix for members is NOT used - just camelCase
- **Constants**: UPPER_SNAKE_CASE for macros, camelCase for constexpr
- **Enums**: PascalCase for enum names and values (e.g., `YuvType::YUV420_NV12`)

### Include Order
1. Project-specific headers (quoted): `"IIPviewer.h"`
2. Qt headers (angled): `<QMainWindow>`, `<QImage>`
3. Standard library (angled): `<array>`, `<memory>`

Example:
```cpp
#include "config.h"
#include "IIPviewer.h"
#include <QApplication>
#include <QMessageBox>
#include <array>
```

### Header Guards
Use `#ifndef`/`#define`/`#endif` style OR `#pragma once`:
```cpp
#ifndef IIPVIEWER_H
#define IIPVIEWER_H
// ... content
#endif // IIPVIEWER_H
```
OR
```cpp
#pragma once
// ... content
```

### Classes
- Mark leaf classes as `final`: `class ImageWidget final : public QWidget`
- Use `explicit` on single-argument constructors
- Use `override` on virtual method overrides
- Delete copy constructor/assignment when appropriate

```cpp
class ImageWidget final : public QWidget
{
    Q_OBJECT
public:
    ImageWidget() = delete;
    explicit ImageWidget(QColor color, int penWidth, QScrollArea *parentScroll, QWidget *parent = nullptr);
    ~ImageWidget();
    void paintEvent(QPaintEvent *event) override;
private:
    QImage *pixMap;
};
```

### Member Initialization
Use member initializer lists and in-class initialization:
```cpp
AppSettings::AppSettings() 
    : yuvType(YuvType::YUV444_INTERLEAVE),
      yuv_bitDepth(8),
      raw_compact(false)
{
}

// Or inline:
class Foo {
    int value = 0;
    QString name;
};
```

### Constants
Use `constexpr` for compile-time constants:
```cpp
constexpr int LEFT_IMG_WIDGET = 0;
constexpr int BIT8 = 8;
constexpr int ZOOM_LIST_LENGTH = 13;
```

### Suppressing Lint Warnings
Use NOLINT comments for intentional violations:
```cpp
int value = 8; // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
QFont font("Microsoft YaHei UI", 10); // NOLINT(cppcoreguidelines-avoid-magic-numbers)
```

### Error Handling
- Use Qt's return-value based error handling pattern
- Check file existence and validity before operations
- Use `QMessageBox` for user-facing errors

```cpp
const QFileInfo fileInfo(path);
if (fileInfo.exists() && fileInfo.isFile())
{
    needOpenFilePath = path;
}
```

### Qt-Specific Patterns
- Use Qt signals/slots mechanism
- Use `Q_OBJECT` macro in classes with signals/slots
- Use `emit` keyword when emitting signals
- Prefer Qt containers (`QString`, `QList`, `QMap`) in Qt code
- Use `Q_UNUSED` or anonymous variable names for unused parameters

### Platform-Specific Code
Use Qt's platform macros:
```cpp
#ifdef Q_OS_WINDOWS
    QApplication::setFont(QFont("Microsoft YaHei UI", 10));
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
- Use `//` for single-line comments, `/* */` for multi-line

## Project-Specific Notes

### Supported Image Formats
- Standard: JPG, BMP, PNG, TIFF
- RAW: 8/10/12/14/16/18/20/22/24 bit, MIPI-RAW (10/12/14 bit), RGBIR-RAW
- YUV: 8/10/12 bit, multiple layouts (444, 422, 420, 400)
- PGM/PNM: 8/10/12/14/16 bit
- HEIF: .heic 8bit YUV 420/422/444

### Key Classes
- `IIPviewer`: Main window class, handles UI and file operations
- `ImageWidget`: Custom widget for image display with zoom/ROI support
- `AppSettings`: Application settings persistence
- `RawFileInfoDlg`, `YuvFileInfoDlg`: Dialogs for format configuration

### Translation
Update translations using lupdate:
```bash
lupdate ./src/*.cpp ./inc/*.h -ts ./translations/IIViewer_zh.ts
```
