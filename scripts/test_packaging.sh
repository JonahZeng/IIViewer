#!/bin/bash

# test_packaging.sh - 测试打包脚本的基本功能

set -e

echo "=== IIViewer打包脚本测试 ==="
echo ""

# 检查脚本是否存在
echo "1. 检查脚本文件..."
SCRIPTS=("create_dmg.sh" "create_pkg.sh" "package_macos.sh")
for script in "${SCRIPTS[@]}"; do
    if [ -f "scripts/$script" ]; then
        echo "  ✓ $script 存在"
    else
        echo "  ✗ $script 不存在"
        exit 1
    fi
done

echo ""

# 检查脚本执行权限
echo "2. 检查脚本执行权限..."
for script in "${SCRIPTS[@]}"; do
    if [ -x "scripts/$script" ]; then
        echo "  ✓ $script 有执行权限"
    else
        echo "  ✗ $script 没有执行权限"
        chmod +x "scripts/$script"
        echo "  ✓ 已添加执行权限"
    fi
done

echo ""

# 测试帮助功能
echo "3. 测试帮助功能..."
./scripts/package_macos.sh --help > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "  ✓ package_macos.sh 帮助功能正常"
else
    echo "  ✗ package_macos.sh 帮助功能异常"
fi

echo ""

# 测试版本号提取
echo "4. 测试版本号提取..."
VERSION_FROM_CMAKE=$(grep -m1 "project(IIViewer VERSION" CMakeLists.txt | sed 's/.*VERSION \([0-9.]*\).*/\1/')
if [ ! -z "$VERSION_FROM_CMAKE" ]; then
    echo "  ✓ 从CMakeLists.txt提取版本号: $VERSION_FROM_CMAKE"
else
    echo "  ✗ 无法从CMakeLists.txt提取版本号"
    VERSION_FROM_CMAKE="0.6.3"
    echo "  ✓ 使用默认版本号: $VERSION_FROM_CMAKE"
fi

echo ""

# 测试DMG脚本参数检查
echo "5. 测试DMG脚本参数检查..."
./scripts/create_dmg.sh 2>&1 | grep -q "用法:"
if [ $? -eq 0 ]; then
    echo "  ✓ create_dmg.sh 参数检查正常"
else
    echo "  ✗ create_dmg.sh 参数检查异常"
fi

echo ""

# 测试PKG脚本参数检查
echo "6. 测试PKG脚本参数检查..."
./scripts/create_pkg.sh 2>&1 | grep -q "用法:"
if [ $? -eq 0 ]; then
    echo "  ✓ create_pkg.sh 参数检查正常"
else
    echo "  ✗ create_pkg.sh 参数检查异常"
fi

echo ""

# 检查必要的工具
echo "7. 检查必要的工具..."
TOOLS=("hdiutil" "codesign")
for tool in "${TOOLS[@]}"; do
    if command -v $tool &> /dev/null; then
        echo "  ✓ $tool 已安装"
    else
        echo "  ✗ $tool 未安装"
    fi
done

# 检查pkgbuild（PKG打包需要）
if command -v pkgbuild &> /dev/null; then
    echo "  ✓ pkgbuild 已安装 (PKG打包可用)"
else
    echo "  ⚠ pkgbuild 未安装 (PKG打包不可用)"
fi

echo ""

# 创建测试目录结构
echo "8. 创建测试目录结构..."
mkdir -p test_build/bin/Release
echo "  ✓ 创建测试目录"

# 创建模拟的.app结构（如果不存在真正的应用程序）
if [ ! -d "bin/Release/IIViewer.app" ]; then
    echo "  ⚠ 注意: 真正的IIViewer.app不存在"
    echo "    打包测试需要先构建应用程序"
    echo "    运行: mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build ."
else
    echo "  ✓ IIViewer.app 存在"
fi

echo ""
echo "=== 测试完成 ==="
echo ""
echo "下一步:"
echo "1. 构建应用程序 (如果尚未构建)"
echo "2. 运行打包测试:"
echo "   ./scripts/package_macos.sh --type dmg --output test_packages"
echo "3. 查看生成的安装包:"
echo "   ls -lh test_packages/"
echo ""
echo "GitHub Actions工作流:"
echo "  当推送v开头的tag时，会自动运行完整打包流程"
echo "  并上传DMG和PKG文件到GitHub Release"