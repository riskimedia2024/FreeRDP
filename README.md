# FreeRDP: A Remote Desktop Protocol Implementation

FreeRDP is a free implementation of the Remote Desktop Protocol (RDP), released under the Apache license.
Enjoy the freedom of using your software wherever you want, the way you want it, in a world where
interoperability can finally liberate your computing experience.

## About This Fork
### Introduction
这是一个完全由个人维护的分支，所有的代码都没有经过严格的测试，不保证这些代码能在所有设备上都可以正确工作。  
该分支拓展了 FreeRDP 的一些功能，但这些拓展通常是作为调试目的而添加的，因此会导致微小的性能损失（这些特性可以   
在命令行中通过指定参数来关闭或启用，部分特性默认是关闭状态）。  

### Features and fixes
该分支基于 2020-07-02 (Version 2.2.0) 的 master 分支。修复了一些在我的设备上出现的数个问题，并给出了一些拓展的功能。    
修复的问题包括：    
+ OpenCL 在复制设备内存到主机内存时的越界问题（导致 Segmentation Fault）。
+ H264 解码器上下文多次创建的问题。

拓展的功能：
+ 在窗口中绘制（矩形框选）最近更新过的区域。
（命令行：+gfx-updated-fragment）
+ 在窗口左上角或标准输出实时显示来自 Decoder 的统计信息（帧率、编码、分辨率等）。
（命令行：+gfx-show-fps-console，+gfx-show-fps-screen）。
+ 当日志输出到标准输出时，进行语法着色。
+ 为每个线程设置自定义名称以便于区分。
+ 自动集成外部代码（如 OpenCL 程序）。

要使用以上全部功能和修复，建议采用如下配置：
+ `WITH_WAYLAND=OFF (Optinal)`
+ `WITH_LIBSYSTEMD=OFF (Optional)`
+ `WITH_OSS=OFF (Optional)`
+ `WITH_ALSA=OFF (Optional)`
+ `WITH_GFX_H264=ON (Required)`
+ `WITH_CAIRO=ON (Required)`
+ `WITH_VAAPI=ON (Recommanded)`
+ `WITH_OPENCL=ON (Recommanded)`
+ `OpenCL_INCLUDE_DIR=<Your OpenCL Header Path>`

若启用 OpenCL，则可使用基于硬件加速（通常是 GPU）的 YUV 到 RGB 的转换；启用 VAAPI 可使用基于硬件加速的 H264 解码。  
凡是基于 GPU 的功能不一定在所有设备上支持，且不一定稳定。   
上述配置之所以建议禁用 OSS, ALSA 的支持，是因为我们希望 FreeRDP 尽量不使用过时的和过于底层的 API。  
~~禁用 libsystemd 仅出于个人喜好。~~    
**部分功能是以项目的跨平台性和强依赖为代价的，因此此分支中我们移除了对 MacOS X, \*BSD, Android, Windows, 以及 iOS** 
**平台的支持（我们也不推荐使用 Wayland，因为并没有经过测试）。**    

#### 已知的问题
1. 在部分使用 Mesa 驱动的 OpenCL 平台上，因为 OpenCL 版本过低导致的编译错误或运行时错误。
可能表现为编译期间的 `clCreateCommandQueueWithProperties()` 未定义，或运行时的「段错误」。
2. 在 GCC 10.2.0 上编译时，在最终链接阶段出现「Undefined reference」错误。

#### 可能的解决方案
1. 使用硬件厂商提供的，版本大于 2.0 的 OpenCL 库。
或手动用上古时代的 `clCreateCommandQueue()` API 来代替上述函数。（文件 libfreerdp/primitives/prim_YUV_opencl.c）
2. 原因暂且不明，但是使用 Clang 可以编译通过。

### About File Resource Integration
「文件资源集成（FRI）」是一个编译时功能。旨在将一些非 C 程序源代码文件的内容作为字符串字面量内嵌到程序中。  
这在处理像 OpenGL/OpenCL 等需要大量运行时编译源代码的程序时非常有用。FRI 使得整个程序不需要在运行时到
特定的路径去加载代码文件，也不需要手动地将外部代码写成字符串（这是一个十分痛苦的过程，并且会失去自动补全
和语法高亮功能）。  
FRI 目前仅仅为 libfreerdp 这个 target 服务，但它是完全通用的。您感兴趣的话完全可以自己拓展其功能。  
FRI 的核心代码十分简单，一个位于 `scripts/FileResourceIntegration.sh` 的 shell 脚本，其配置文件
和生成的文件位于 `resources/generated`。配置文件指导 FRI 脚本为哪些文件执行操作，并生成什么符号。   
生成的文件是 `dynamic_buildtime_fri.c`，它会参与 libfreerdp 的编译链接过程。`Buildtime.h` 定义
了 C 源代码与生成的符号之间的接口（若干个宏定义），使用 `USE_FILE_INTEGRATION_SYMBOL(sym)` 宏来
引用在配置文件中定义的符号，符号会被自动重命名以最大限度防止冲突，因此不需要特别注意如何设置符号名
使其不与已存在的符号冲突。  
需要特别注意的是，一个符号需要在两个地方定义：
1. FRI 的配置文件 `FileResourceIntegration.conf` 中定义
2. 使用 `DECLARE_FILE_INTEGRATION_STRING(sym)` 宏在 `Buildtime.h` 中定义

## Resources

注意，以下是原 FreeRDP 项目给出的 References，我们不再保证这些文档在此版本中仍然全部或局部适用。

Project website: https://www.freerdp.com/

Issue tracker: https://github.com/FreeRDP/FreeRDP/issues

Sources: https://github.com/FreeRDP/FreeRDP/

Downloads: https://pub.freerdp.com/releases/

Wiki: https://github.com/FreeRDP/FreeRDP/wiki

API documentation: https://pub.freerdp.com/api/

IRC channel: #freerdp @ irc.freenode.net

Mailing list: https://lists.sourceforge.net/lists/listinfo/freerdp-devel

## Microsoft Open Specifications

Information regarding the Microsoft Open Specifications can be found at:
http://www.microsoft.com/openspecifications/

A list of reference documentation is maintained here:
https://github.com/FreeRDP/FreeRDP/wiki/Reference-Documentation

## Compilation

### Compilation Instructions
```shell
# Out-of-source build
mkdir build
cd build
# Configuration (Must use clang now)
cmake \
    -DCMAKE_C_COMPILER=clang \
    -DWITH_WAYLAND=OFF \
    -DWITH_LIBSYSTEMD=OFF \
    -DWITH_OSS=OFF \
    -DWITH_ALSA=OFF \
    -DWITH_GFX_H264=ON \
    -DWITH_CAIRO=ON \
    -DWITH_VAAPI=ON \
    -DWITH_OPENCL=ON \
    -DOpenCL_INCLUDE_DIR=<OpenCL include> \
    ..
# Build (N can be the number of CPU cores)
# Or you can use Ninja instead of GNU Make, just specify -G Ninja when configure.
make -j N
```

### Reference
Instructions on how to get started compiling FreeRDP can be found on the wiki:  
https://github.com/FreeRDP/FreeRDP/wiki/Compilation
