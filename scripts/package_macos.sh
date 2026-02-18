#!/bin/bash

# package_macos.sh - IIViewer macOS打包主脚本
# 用法: ./package_macos.sh [选项]

set -e

# 默认值
APP_NAME="IIViewer"
VERSION="0.6.3"
BUILD_TYPE="Release"
PACKAGE_TYPE="dmg"  # dmg, pkg, all
OUTPUT_DIR="dist"
APP_PATH="bin/${BUILD_TYPE}/${APP_NAME}.app"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}"
    echo "========================================"
    echo "    IIViewer macOS 打包工具"
    echo "========================================"
    echo -e "${NC}"
}

print_usage() {
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -t, --type TYPE     打包类型: dmg, pkg, all (默认: dmg)"
    echo "  -v, --version VER   版本号 (默认: 从CMakeLists.txt读取)"
    echo "  -b, --build TYPE    构建类型: Release, Debug (默认: Release)"
    echo "  -o, --output DIR    输出目录 (默认: dist)"
    echo "  -a, --app PATH      应用程序路径"
    echo "  -h, --help          显示帮助信息"
    echo ""
    echo "示例:"
    echo "  $0 --type dmg --version 1.0.0"
    echo "  $0 --type all --output packages"
    echo "  $0 --app bin/Release/IIViewer.app --type pkg"
}

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            PACKAGE_TYPE="$2"
            shift 2
            ;;
        -v|--version)
            VERSION="$2"
            shift 2
            ;;
        -b|--build)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -o|--output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -a|--app)
            APP_PATH="$2"
            shift 2
            ;;
        -h|--help)
            print_header
            print_usage
            exit 0
            ;;
        *)
            echo "未知选项: $1"
            print_usage
            exit 1
            ;;
    esac
done

# 从CMakeLists.txt读取版本号（如果未指定）
if [ "$VERSION" = "0.6.3" ]; then
    if [ -f "CMakeLists.txt" ]; then
        EXTRACTED_VERSION=$(grep -m1 "project(IIViewer VERSION" CMakeLists.txt | sed 's/.*VERSION \([0-9.]*\).*/\1/')
        if [ ! -z "$EXTRACTED_VERSION" ]; then
            VERSION="$EXTRACTED_VERSION"
            echo -e "${GREEN}从CMakeLists.txt读取版本号: $VERSION${NC}"
        fi
    fi
fi

# 更新应用程序路径（如果使用默认值）
if [ "$APP_PATH" = "bin/${BUILD_TYPE}/${APP_NAME}.app" ]; then
    APP_PATH="bin/${BUILD_TYPE}/${APP_NAME}.app"
fi

print_header

# 检查应用程序是否存在
if [ ! -d "$APP_PATH" ]; then
    echo -e "${RED}错误: 应用程序不存在: $APP_PATH${NC}"
    echo "请先构建应用程序或使用 --app 参数指定路径"
    exit 1
fi

echo -e "${GREEN}打包配置:${NC}"
echo "  应用程序: $APP_PATH"
echo "  版本号: $VERSION"
echo "  打包类型: $PACKAGE_TYPE"
echo "  输出目录: $OUTPUT_DIR"
echo ""

# 创建输出目录
mkdir -p "$OUTPUT_DIR"

# 打包函数
package_dmg() {
    echo -e "${YELLOW}创建DMG安装包...${NC}"
    
    # 尝试使用高级脚本
    if [ -f "scripts/create_dmg_advanced.sh" ]; then
        ./scripts/create_dmg_advanced.sh "$APP_PATH" "$VERSION" "$OUTPUT_DIR"
    elif [ -f "scripts/create_dmg.sh" ]; then
        ./scripts/create_dmg.sh "$APP_PATH" "$VERSION" "$OUTPUT_DIR"
    else
        echo -e "${RED}错误: 找不到DMG创建脚本${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✓ DMG创建完成${NC}"
}

package_pkg() {
    echo -e "${YELLOW}创建PKG安装包...${NC}"
    
    # 检查pkgbuild命令是否可用
    if ! command -v pkgbuild &> /dev/null; then
        echo -e "${RED}错误: pkgbuild命令未找到${NC}"
        echo "请确保已安装Xcode命令行工具: xcode-select --install"
        exit 1
    fi
    
    if [ -f "scripts/create_pkg.sh" ]; then
        ./scripts/create_pkg.sh "$APP_PATH" "$VERSION" "$OUTPUT_DIR"
    else
        echo -e "${RED}错误: 找不到PKG创建脚本${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✓ PKG创建完成${NC}"
}

# 执行打包
case $PACKAGE_TYPE in
    dmg)
        package_dmg
        ;;
    pkg)
        package_pkg
        ;;
    all)
        package_dmg
        echo ""
        package_pkg
        ;;
    *)
        echo -e "${RED}错误: 不支持的打包类型: $PACKAGE_TYPE${NC}"
        echo "支持的类型: dmg, pkg, all"
        exit 1
        ;;
esac

# 显示结果
echo ""
echo -e "${BLUE}========================================"
echo "    打包完成!"
echo "========================================"
echo -e "${NC}"

# 列出生成的文件
echo -e "${GREEN}生成的文件:${NC}"
ls -lh "$OUTPUT_DIR"/*.dmg "$OUTPUT_DIR"/*.pkg 2>/dev/null | awk '{print "  " $0}'

echo ""
echo -e "${YELLOW}下一步:${NC}"
echo "  1. 将安装包分发给用户"
echo "  2. 用户双击安装包进行安装"
echo "  3. 将应用程序拖到Applications文件夹中"
echo ""
echo -e "${GREEN}完成!${NC}"