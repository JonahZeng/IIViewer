#!/bin/bash

# create_dmg_advanced.sh - 创建带背景图片的DMG安装包
# 需要安装: npm install -g create-dmg (可选)

set -e

# 参数检查
if [ $# -lt 3 ]; then
    echo "用法: $0 <app_path> <version> <output_dir>"
    echo "示例: $0 bin/Release/IIViewer.app 0.6.3 dist"
    exit 1
fi

APP_PATH="$1"
VERSION="$2"
OUTPUT_DIR="$3"
APP_NAME="IIViewer"
DMG_NAME="${APP_NAME}-${VERSION}-macos-aarch64"
VOLUME_NAME="${APP_NAME} ${VERSION}"
DMG_PATH="${OUTPUT_DIR}/${DMG_NAME}.dmg"

# 检查输入文件
if [ ! -d "$APP_PATH" ]; then
    echo "错误: 应用程序不存在: $APP_PATH"
    exit 1
fi

# 创建输出目录
mkdir -p "$OUTPUT_DIR"

# 方法1: 使用create-dmg（如果已安装）
if command -v create-dmg &> /dev/null; then
    echo "使用create-dmg创建带背景的DMG..."
    
    # 创建临时背景图片（如果没有的话）
    BACKGROUND_DIR="$OUTPUT_DIR/dmg_background"
    mkdir -p "$BACKGROUND_DIR"
    
    # 检查是否有背景图片
    if [ -f "icon/dmg_background.png" ]; then
        BACKGROUND_IMAGE="icon/dmg_background.png"
    else
        # 创建简单的背景图片
        echo "创建默认背景图片..."
        convert -size 600x400 gradient:blue-cyan "$BACKGROUND_DIR/background.png" 2>/dev/null || \
        echo "注意: ImageMagick未安装，使用纯色背景"
        BACKGROUND_IMAGE="$BACKGROUND_DIR/background.png"
    fi
    
    create-dmg \
        --volname "$VOLUME_NAME" \
        --volicon "icon/IIViewer.icns" \
        --background "$BACKGROUND_IMAGE" \
        --window-pos 200 120 \
        --window-size 600 400 \
        --icon-size 100 \
        --icon "$APP_NAME.app" 100 190 \
        --hide-extension "$APP_NAME.app" \
        --app-drop-link 380 190 \
        --no-internet-enable \
        "$DMG_PATH" \
        "$APP_PATH"
    
    if [ $? -eq 0 ]; then
        echo "✓ create-dmg创建成功: $DMG_PATH"
    else
        echo "✗ create-dmg创建失败，尝试使用hdiutil..."
        METHOD="hdiutil"
    fi
else
    echo "create-dmg未安装，使用hdiutil..."
    METHOD="hdiutil"
fi

# 方法2: 使用hdiutil（回退方案）
if [ "$METHOD" = "hdiutil" ] || [ -z "$METHOD" ]; then
    echo "使用hdiutil创建DMG..."
    
    # 创建临时目录
    TEMP_DIR=$(mktemp -d)
    echo "临时目录: $TEMP_DIR"
    
    # 复制应用程序到临时目录
    cp -R "$APP_PATH" "$TEMP_DIR/"
    
    # 创建Applications文件夹链接
    ln -s /Applications "$TEMP_DIR/Applications"
    
    # 创建DMG
    hdiutil create \
        -volname "$VOLUME_NAME" \
        -srcfolder "$TEMP_DIR" \
        -ov \
        -format UDZO \
        -imagekey zlib-level=9 \
        "$DMG_PATH"
    
    # 清理临时目录
    rm -rf "$TEMP_DIR"
    
    if [ $? -eq 0 ]; then
        echo "✓ hdiutil创建成功: $DMG_PATH"
    else
        echo "✗ DMG创建失败"
        exit 1
    fi
fi

# 获取DMG文件信息
DMG_SIZE=$(du -h "$DMG_PATH" | cut -f1)
echo ""
echo "打包完成！"
echo "================"
echo "应用名称: $APP_NAME"
echo "版本: $VERSION"
echo "DMG文件: $DMG_PATH"
echo "文件大小: $DMG_SIZE"
echo "================"
echo ""
echo "安装说明:"
echo "1. 双击打开 $DMG_NAME.dmg"
echo "2. 将 $APP_NAME.app 拖到 Applications 文件夹"
echo "3. 在Launchpad或Applications文件夹中启动 $APP_NAME"