# IIViewer macOS 打包脚本

这个目录包含了用于将IIViewer应用程序打包为macOS可分发包的脚本。

## 脚本说明

### 1. `create_dmg.sh` - 基础DMG创建脚本
```bash
# 用法
./create_dmg.sh <app_path> <version> <output_dir>

# 示例
./create_dmg.sh bin/Release/IIViewer.app 0.6.3 dist
```

### 2. `create_dmg_advanced.sh` - 高级DMG创建脚本
支持背景图片和更好的布局，需要安装`create-dmg`工具：
```bash
# 安装create-dmg
npm install -g create-dmg

# 用法
./create_dmg_advanced.sh <app_path> <version> <output_dir>
```

### 3. `create_pkg.sh` - PKG安装包创建脚本
```bash
# 用法
./create_pkg.sh <app_path> <version> <output_dir>

# 示例
./create_pkg.sh bin/Release/IIViewer.app 0.6.3 dist
```

### 4. `package_macos.sh` - 主打包脚本（推荐）
```bash
# 显示帮助
./package_macos.sh --help

# 创建DMG包
./package_macos.sh --type dmg

# 创建PKG包
./package_macos.sh --type pkg

# 创建所有包
./package_macos.sh --type all

# 指定版本和输出目录
./package_macos.sh --type dmg --version 1.0.0 --output packages
```

## 在GitHub Actions中的使用

这些脚本已经集成到 `.github/workflows/release-macos_arm64.yml` 工作流中。当推送一个以 `v` 开头的tag时（例如 `v0.6.3`），工作流会自动：

1. 构建应用程序
2. 使用macdeployqt打包依赖
3. 创建DMG和PKG安装包
4. 上传到GitHub Release

## 本地测试

### 前提条件
1. 确保应用程序已经构建完成：`bin/Release/IIViewer.app`
2. 确保脚本有执行权限：
   ```bash
   chmod +x scripts/*.sh
   ```

### 测试DMG创建
```bash
# 创建dist目录
mkdir -p dist

# 运行DMG打包
./scripts/package_macos.sh --type dmg

# 或者直接使用基础脚本
./scripts/create_dmg.sh bin/Release/IIViewer.app 0.6.3 dist
```

### 测试PKG创建
```bash
# 确保已安装Xcode命令行工具
xcode-select --install

# 运行PKG打包
./scripts/package_macos.sh --type pkg
```

## 生成的安装包

### DMG文件
- 文件名：`IIViewer-<version>-macos.dmg`
- 用户双击打开后，可以将IIViewer.app拖到Applications文件夹中安装

### PKG文件
- 文件名：`IIViewer-<version>.pkg`
- 用户双击运行安装向导，自动安装到Applications文件夹

## 自定义配置

### 背景图片
要使用自定义背景图片，将图片放在 `icon/dmg_background.png`，高级脚本会自动使用它。

### 版本号
脚本会自动从CMakeLists.txt读取版本号，也可以通过命令行参数指定。

## 故障排除

### 1. "pkgbuild: command not found"
确保已安装Xcode命令行工具：
```bash
xcode-select --install
```

### 2. "create-dmg: command not found"
安装create-dmg工具：
```bash
npm install -g create-dmg
```

### 3. 权限问题
确保脚本有执行权限：
```bash
chmod +x scripts/*.sh
```

### 4. 应用程序不存在
确保先构建应用程序：
```bash
# 在项目根目录
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## 注意事项

1. **代码签名**：GitHub Actions工作流中使用自签名证书。如果要上架App Store或进行公证，需要正式的开发者证书。
2. **依赖库**：macdeployqt会自动处理Qt依赖，但其他第三方库（如libheif）需要静态链接或手动包含。
3. **文件大小**：DMG和PKG文件可能较大，因为包含了所有依赖库。