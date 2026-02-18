#!/bin/bash

# create_dmg.sh - 创建IIViewer的DMG安装包
# 用法: ./create_dmg.sh <app_path> <version> <output_dir>

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
DMG_NAME="${APP_NAME}-${VERSION}-macos"
VOLUME_NAME="${APP_NAME} ${VERSION}"
DMG_PATH="${OUTPUT_DIR}/${DMG_NAME}.dmg"

# 检查输入文件
if [ ! -d "$APP_PATH" ]; then
    echo "错误: 应用程序不存在: $APP_PATH"
    exit 1
fi

# 创建输出目录
mkdir -p "$OUTPUT_DIR"

# 创建临时目录
TEMP_DIR=$(mktemp -d)
echo "临时目录: $TEMP_DIR"

# 复制应用程序到临时目录
cp -R "$APP_PATH" "$TEMP_DIR/"

# 创建Applications文件夹链接
ln -s /Applications "$TEMP_DIR/Applications"

# 设置背景图片（可选）
# 如果有背景图片，可以在这里添加
# BACKGROUND_IMAGE="path/to/background.png"

# 创建DMG
echo "创建DMG: $DMG_PATH"
hdiutil create \
    -volname "$VOLUME_NAME" \
    -srcfolder "$TEMP_DIR" \
    -ov \
    -format UDZO \
    -imagekey zlib-level=9 \
    "$DMG_PATH"

# 检查DMG是否创建成功
if [ $? -eq 0 ]; then
    echo "✓ DMG创建成功: $DMG_PATH"
    
    # 获取DMG文件信息
    DMG_SIZE=$(du -h "$DMG_PATH" | cut -f1)
    echo "  - 文件大小: $DMG_SIZE"
    echo "  - 版本: $VERSION"
    
    # 可选：添加图标到DMG
    # hdiutil attach "$DMG_PATH"
    # 设置图标等操作
    # hdiutil detach /Volumes/"$VOLUME_NAME"
else
    echo "✗ DMG创建失败"
    exit 1
fi

# 清理临时目录
rm -rf "$TEMP_DIR"
echo "临时目录已清理"

echo ""
echo "打包完成！"
echo "DMG文件: $DMG_PATH"
echo "用户可以将$APP_NAME拖到Applications文件夹中安装。"