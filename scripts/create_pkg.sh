#!/bin/bash

# create_pkg.sh - 创建IIViewer的PKG安装包
# 用法: ./create_pkg.sh <app_path> <version> <output_dir>

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
PKG_NAME="${APP_NAME}-${VERSION}.pkg"
PKG_PATH="${OUTPUT_DIR}/${PKG_NAME}"

# 检查输入文件
if [ ! -d "$APP_PATH" ]; then
    echo "错误: 应用程序不存在: $APP_PATH"
    exit 1
fi

# 创建输出目录
mkdir -p "$OUTPUT_DIR"

# 创建组件包
echo "创建组件包..."
COMPONENT_PKG="${OUTPUT_DIR}/component.pkg"
pkgbuild \
    --component "$APP_PATH" \
    --install-location "/Applications" \
    --identifier "com.${APP_NAME}.app" \
    --version "$VERSION" \
    "$COMPONENT_PKG"

if [ $? -ne 0 ]; then
    echo "✗ 组件包创建失败"
    exit 1
fi

# 创建分发文件
echo "创建分发文件..."
DISTRIBUTION_FILE="${OUTPUT_DIR}/distribution.xml"
cat > "$DISTRIBUTION_FILE" << EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>${APP_NAME} ${VERSION}</title>
    <organization>com.${APP_NAME}</organization>
    <domains enable_localSystem="true"/>
    <options customize="never" require-scripts="false"/>
    <volume-check>
        <allowed-os-versions>
            <os-version min="10.13"/>
        </allowed-os-versions>
    </volume-check>
    <choices-outline>
        <line choice="default">
            <line choice="${APP_NAME}"/>
        </line>
    </choices-outline>
    <choice id="default"/>
    <choice id="${APP_NAME}" title="${APP_NAME}" description="Install ${APP_NAME} ${VERSION}">
        <pkg-ref id="com.${APP_NAME}.app"/>
    </choice>
    <pkg-ref id="com.${APP_NAME}.app" version="$VERSION" onConclusion="none">component.pkg</pkg-ref>
</installer-gui-script>
EOF

# 创建产品包
echo "创建产品包: $PKG_PATH"
productbuild \
    --distribution "$DISTRIBUTION_FILE" \
    --package-path "$OUTPUT_DIR" \
    --resources "." \
    "$PKG_PATH"

if [ $? -eq 0 ]; then
    echo "✓ PKG创建成功: $PKG_PATH"
    
    # 获取PKG文件信息
    PKG_SIZE=$(du -h "$PKG_PATH" | cut -f1)
    echo "  - 文件大小: $PKG_SIZE"
    echo "  - 版本: $VERSION"
    
    # 清理临时文件
    rm -f "$COMPONENT_PKG" "$DISTRIBUTION_FILE"
    echo "临时文件已清理"
else
    echo "✗ PKG创建失败"
    exit 1
fi

echo ""
echo "打包完成！"
echo "PKG文件: $PKG_PATH"
echo ""
echo "安装说明:"
echo "1. 双击 $PKG_NAME 开始安装"
echo "2. 按照安装向导完成安装"
echo "3. 在Applications文件夹中启动 $APP_NAME"