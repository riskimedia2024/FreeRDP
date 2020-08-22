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

#### 可能的解决方案
1. 使用硬件厂商提供的，版本大于 2.0 的 OpenCL 库。
或手动用上古时代的 `clCreateCommandQueue()` API 来代替上述函数。（文件 libfreerdp/primitives/prim_YUV_opencl.c）

### Code References
GFX 解码器审计拓展（显示 FPS 等实时性能数据）：
```c
typedef struct { ... } GFXAuditContext;

static char __text_paint_buffer[...][...];

static void __gfx_audit_setup_cairo(GFXAuditContext *ctx, int w, int h);
static void __gfx_audit_resize_cairo(GFXAuditContext *ctx, int w, int h);

GFXAuditContext *GFXAuditNewContext(void *gfxClientContext);

void GFXAuditReleaseContext(GFXAuditContext *ctx);
void GFXAuditSetDecoderName(GFXAuditContext *ctx, char const *name);
void GFXAuditDecodeStartTimer(GFXAuditContext *ctx);
void GFXAuditDecodeStopTimer(GFXAuditContext *ctx);
void GFXAuditFrameStartTimer(GFXAuditContext *ctx);
void GFXAuditFrameStopTimer(GFXAuditContext *ctx);

static void __gfx_audit_show_console_fps(GFXAuditContext *ctx, BOOL repaint);
static void __gfx_audit_screen_render(GFXAuditContext *ctx, gdiGfxSurface *gdi);
static void __gfx_audit_screen_paint(GFXAuditContext *ctx, gdiGfxSurface *surface);
static void __gfx_audit_show_screen_fps(GFXAuditContext *ctx, gdiGfxSurface *surface, BOOL repaint);
static void __gfx_audit_show_fps(GFXAuditContext *ctx, RdpgfxClientContext *gfx, gdiGfxSurface *surface);

void GFXAuditAccept(GFXAuditContext *ctx, void *dstSurface);
void GFXAuditPaintScreen(GFXAuditContext *ctx, void *surface);

static void __gfx_audit_paint_horizontal_line(UINT32 *__ptr, int sx, int sy, int sw, int sh, int len, int height);
static void __gfx_audit_paint_vertical_line(UINT32 *__ptr, int sx, int sy, int sw, int sh, int len, int width);

void GFXAuditPaintUpdated(BYTE *pDst, const RECTANGLE_16 *regions, UINT32 num, UINT32 nDstWidth, UINT32 nDstHeight);
```

日志语法着色拓展：
```c
typedef enum { ... } Identifier;
typedef enum { ... } Colors;

typedef struct { ... } ColorAttributeMap;
typedef struct { ... } IdentColorMap;
typedef struct { ... } CatchBlock;

typedef struct { ... } LevelStringMap;

static const ColorAttributeMap __cshader_camap[] = { ... };
static const IdentColorMap __cshader_icmap[] = { ... };
static const LevelStringMap __cshader_lsmap[] = { ... };

_Bool __cshader_matches_catchblock(char const *str, CatchBlock *catchblock);

_Bool __cshader_is_number(char ch);
_Bool __cshader_is_upper(char ch);
_Bool __cshader_is_lower(char ch);
char __cshader_to_lower(char ch);

_Bool __cshader_case_insensitive_compare_equal(const char *begin, const char *end, char const *str);

_Bool __cshader_is_integer(char const *begin, char const *end);
_Bool __cshader_is_float(char const *begin, char const *end);
_Bool __cshader_is_llegal_identifier(char const *begin, char const *end);

_Bool __cshader_check_number(CatchBlock *cb);
_Bool __cshader_check_timestamp(CatchBlock *cb);
_Bool __cshader_check_level(CatchBlock *cb);
_Bool __cshader_check_modulename(CatchBlock *cb);

void __cshader_catchblock_tochecked(CatchBlock *cb);
void __cshader_apply(FILE *fp, CatchBlock *cb);
void __cshader_transport(FILE *fp, char const *str);

void CsShaderTransport(FILE *fp, char const *prefix, char const *content);
```

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

Instructions on how to get started compiling FreeRDP can be found on the wiki:
https://github.com/FreeRDP/FreeRDP/wiki/Compilation
