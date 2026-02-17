这个是一个基于ffplay改的一个Qt播放器(支持ubuntu),简洁,结构简单,稳定,正确实现了大多数基本功能(快进,后退,暂停,停止,变速)。自动优先选择硬解码。使用的是qt6.7.3,qt6应该都没问题
This is a Qt-based media player derived from ffplay, featuring a clean, lightweight, and stable architecture that properly implements essential playback functions (fast-forward, rewind, pause, stop, and speed adjustment). It automatically prioritizes hardware decoding when available. Built with Qt 6.7.3 (fully compatible with Qt6).

下面介绍如何在 Ubuntu 系统上编译本播放器项目（基于 Qt6 + FFmpeg + SDL2 + SoundTouch + GLM）.
编译器：GCC 9+（支持 C++17）

CMake 3.16+

Git

1. 安装依赖库
1.1 Qt 6.7.3
推荐使用 Qt 官方在线安装器安装 Qt 6.7.3 的桌面版（gcc 64-bit）。

bash
# 下载安装器（可从 Qt 官网获取）
wget https://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run
chmod +x qt-unified-linux-x64-online.run
./qt-unified-linux-x64-online.run
按照图形界面提示，登录 Qt 账户，选择安装路径（默认 ~/Qt），并勾选 Qt 6.7.3 下的 Desktop gcc 64-bit 组件。安装完成后，Qt 会被安装到 ~/Qt/6.7.3/gcc_64。

注意：如果使用其他路径，后续 CMake 配置时需通过 -DQt6_ROOT=你的路径 指定。

1.2 FFmpeg（6.x 版本）
建议从源码编译 FFmpeg 6.x 并安装到自定义目录（如 /opt/ffmpeg6.1.4），以避免系统包版本过新导致 API 不兼容。

bash
# 下载 FFmpeg 6.1.4 源码
wget https://ffmpeg.org/releases/ffmpeg-6.1.4.tar.gz
tar -xzf ffmpeg-6.1.4.tar.gz
cd ffmpeg-6.1.4

# 配置、编译、安装（可根据需要添加更多编解码器支持）
./configure --prefix=/opt/ffmpeg6.1.4 --enable-shared --disable-static --enable-gpl --enable-libx264 --enable-libx265 --enable-libmp3lame --enable-libvorbis --enable-libvpx --enable-libdav1d --enable-libopus --enable-vaapi \          # 启用 VA-API（Intel）
            --enable-libv4l2 \         # 启用 V4L2（可选）
            --enable-cuda-nvcc \       # 如果使用 NVIDIA，启用 CUDA 支持
            --enable-nvenc \           # 如果使用 NVIDIA，启用 NVENC
            --enable-libnpp
make -j$(nproc)
sudo make install

上面ffmpeg开启这些选项所依赖的需要自行安装
编译完成后，将 FFmpeg 的 pkgconfig 路径加入环境变量（仅用于编译时查找）：

bash
export PKG_CONFIG_PATH=/opt/ffmpeg6.1.4/lib/pkgconfig:$PKG_CONFIG_PATH

1.3 SDL2
通过 apt 安装即可：

bash
sudo apt update
sudo apt install libsdl2-dev
1.4 SoundTouch
项目已包含 SoundTouch 2.4.0 源码，位于 3rdparty/soundtouch 目录，并已配置为子项目自动编译（开启 C 接口）。无需单独安装。

1.5 GLM
GLM 是 header-only 数学库，项目已将源码放在 glm 目录下，无需额外安装。
3. 配置和编译
3.1 设置环境变量（临时）
bash
export PKG_CONFIG_PATH=/opt/ffmpeg6.1.4/lib/pkgconfig:$PKG_CONFIG_PATH
3.2 创建构建目录并配置 CMake
bash
cd /your path/QtFfplayer
mkdir build && cd build
cmake -DQt6_ROOT=~/Qt/6.7.3/gcc_64 ..
如果 Qt 安装在其他位置，请相应修改 -DQt6_ROOT。

3.3 编译
bash
make -j$(nproc)
编译完成后，可执行文件 QtFfplayer 将出现在 build 目录中。
Compiling the Player Project on Ubuntu (Qt6 + FFmpeg + SDL2 + SoundTouch + GLM)
This document describes how to compile this player project on Ubuntu.

Requirements
Compiler: GCC 9+ (with C++17 support)

CMake: 3.16+

Git

1. Installing Dependencies
1.1 Qt 6.7.3
It is recommended to install the desktop version (gcc 64-bit) of Qt 6.7.3 using the official Qt online installer.

bash
# Download the installer (available from the Qt website)
wget https://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run
chmod +x qt-unified-linux-x64-online.run
./qt-unified-linux-x64-online.run
Follow the graphical interface prompts: log in to your Qt account, choose the installation path (default is ~/Qt), and select the Desktop gcc 64-bit component under Qt 6.7.3. After installation, Qt will be located at ~/Qt/6.7.3/gcc_64.

Note: If you use a different path, you must specify it later during CMake configuration with -DQt6_ROOT=your_path.

1.2 FFmpeg (version 6.x)
It is recommended to compile FFmpeg 6.x from source and install it to a custom directory (e.g., /opt/ffmpeg6.1.4) to avoid API incompatibilities caused by overly new system packages.

bash
# Download FFmpeg 6.1.4 source code
wget https://ffmpeg.org/releases/ffmpeg-6.1.4.tar.gz
tar -xzf ffmpeg-6.1.4.tar.gz
cd ffmpeg-6.1.4

# Configure, compile, and install (add more codec support as needed)
./configure --prefix=/opt/ffmpeg6.1.4 \
            --enable-shared \
            --disable-static \
            --enable-gpl \
            --enable-libx264 \
            --enable-libx265 \
            --enable-libmp3lame \
            --enable-libvorbis \
            --enable-libvpx \
            --enable-libdav1d \
            --enable-libopus \
            --enable-vaapi \          # Enable VA-API (Intel/AMD)
            --enable-libv4l2 \         # Enable V4L2 (optional)
            --enable-cuda-nvcc \       # If using NVIDIA, enable CUDA support
            --enable-nvenc \           # If using NVIDIA, enable NVENC
            --enable-libnpp
make -j$(nproc)
sudo make install
Note: Enabling the above options may require additional dependencies; install them as needed.

After compilation, add the FFmpeg pkgconfig path to your environment variable (only needed during compilation):

bash
export PKG_CONFIG_PATH=/opt/ffmpeg6.1.4/lib/pkgconfig:$PKG_CONFIG_PATH
1.3 SDL2
SDL2 can be installed via apt:

bash
sudo apt update
sudo apt install libsdl2-dev
1.4 SoundTouch
The project already includes the SoundTouch 2.4.0 source code in the 3rdparty/soundtouch directory and is configured as a subproject to be compiled automatically (with the C interface enabled). No separate installation is required.

1.5 GLM
GLM is a header-only math library. Its source code is placed in the glm directory, so no additional installation is needed.

2. Configuration and Compilation
2.1 Set Environment Variables (Temporary)
bash
export PKG_CONFIG_PATH=/opt/ffmpeg6.1.4/lib/pkgconfig:$PKG_CONFIG_PATH
2.2 Create Build Directory and Configure CMake
bash
cd /your/path/QtFfplayer
mkdir build && cd build
cmake -DQt6_ROOT=~/Qt/6.7.3/gcc_64 ..
If Qt is installed elsewhere, adjust -DQt6_ROOT accordingly.

2.3 Compile
bash
make -j$(nproc)
After compilation, the executable QtFfplayer will be located in the build directory.

