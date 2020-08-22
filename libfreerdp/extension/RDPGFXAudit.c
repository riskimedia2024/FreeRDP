#include <freerdp/extension/RDPGFXAudit.h>
#include <freerdp/client/rdpgfx.h>
#include <freerdp/gdi/gfx.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

static void __gfx_audit_setup_cairo(GFXAuditContext *ctx, int w, int h)
{
    ctx->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    ctx->cairo = cairo_create(ctx->surface);

    cairo_select_font_face(ctx->cairo, "Consolas", CAIRO_FONT_SLANT_NORMAL,
        CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(ctx->cairo, 13);
    cairo_set_antialias(ctx->cairo, CAIRO_ANTIALIAS_BEST);
}

static void __gfx_audit_resize_cairo(GFXAuditContext *ctx, int w, int h)
{
    cairo_destroy(ctx->cairo);
    cairo_surface_destroy(ctx->surface);
    __gfx_audit_setup_cairo(ctx, w, h);
}

void __gfx_audit_region16_direct_dump(const REGION16* region)
{
	const RECTANGLE_16* rects;
	UINT32 nbRects, i;
	int currentBandY = -1;
	rects = region16_rects(region, &nbRects);
	printf("nrects=%" PRIu32 "\n", nbRects);

	for (i = 0; i < nbRects; i++, rects++)
	{
		if (rects->top != currentBandY)
		{
			currentBandY = rects->top;
			printf("band %d: ", currentBandY);
		}

		printf("(%" PRIu16 ",%" PRIu16 "-%" PRIu16 ",%" PRIu16 ")\n", rects->left, rects->top,
		         rects->right, rects->bottom);
	}
}

GFXAuditContext *GFXAuditNewContext(void *gfxClientContext)
{
    GFXAuditContext *ctx = (GFXAuditContext *) malloc(sizeof(GFXAuditContext));
    ctx->gfxContext = gfxClientContext;
    ctx->decoderName = "Unknown";
    __gfx_audit_setup_cairo(ctx, CAIRO_AUDIT_SURFACE_W, CAIRO_AUDIT_SURFACE_H);

    ftime(&ctx->fps_timer.st);
    ctx->frames = 0;
    ctx->fps = 0;
    ctx->primitives = primitives_get();

    // RdpgfxClientContext *gfx = (RdpgfxClientContext *)gfxClientContext;

    return ctx;
}

void GFXAuditReleaseContext(GFXAuditContext *ctx)
{
    if (ctx)
    {
        cairo_destroy(ctx->cairo);
        cairo_surface_destroy(ctx->surface);
        free(ctx);
    }
}

void GFXAuditSetDecoderName(GFXAuditContext *ctx, char const *name)
{
    ctx->decoderName = name;
}

void GFXAuditDecodeStartTimer(GFXAuditContext *ctx)
{
    ftime(&ctx->decode_timer.st);
}

void GFXAuditDecodeStopTimer(GFXAuditContext *ctx)
{
    ftime(&ctx->decode_timer.et);
    struct timeb *st = &ctx->decode_timer.st;
    struct timeb *et = &ctx->decode_timer.et;
    ctx->decode_timer.dt = (et->time - st->time) * 1000
        + (et->millitm - st->millitm);
}

void GFXAuditFrameStartTimer(GFXAuditContext *ctx)
{
    ftime(&ctx->frame_timer.st);
}

void GFXAuditFrameStopTimer(GFXAuditContext *ctx)
{
    ftime(&ctx->frame_timer.et);
    struct timeb *st = &ctx->frame_timer.st;
    struct timeb *et = &ctx->frame_timer.et;
    ctx->frame_timer.dt = (et->time - st->time) * 1000
        + (et->millitm - st->millitm);
}

static void __gfx_audit_show_console_fps(GFXAuditContext *ctx, BOOL repaint)
{
    if (repaint)
    {
        printf("\r\033[K%s: %3.2f FPS", ctx->decoderName, ctx->fps);
        fflush(stdout);
    }
}

static char __text_paint_buffer[6][256];
static void __gfx_audit_screen_render(GFXAuditContext *ctx, gdiGfxSurface *gdi)
{
    cairo_text_extents_t text;
    snprintf(__text_paint_buffer[0], 256, "GFX Report");
    snprintf(__text_paint_buffer[1], 256, "Resolution: %dx%d", gdi->width, gdi->height);
    snprintf(__text_paint_buffer[2], 256, "Decoder: %s", ctx->decoderName);
    snprintf(__text_paint_buffer[3], 256, "Receive: %lu ms", ctx->frame_timer.dt);
    snprintf(__text_paint_buffer[4], 256, "Decode: %lu ms", ctx->decode_timer.dt);
    snprintf(__text_paint_buffer[5], 256, "FPS: %2.2f", ctx->fps);

    cairo_set_source_rgb(ctx->cairo, 0.30, 0.72, 0.56);
    cairo_paint(ctx->cairo);
    cairo_set_source_rgb(ctx->cairo, 0.1, 0.1, 0.1);

    BOOL first = TRUE;
    double px, py;
    int h = cairo_image_surface_get_height(ctx->surface);
    int w = cairo_image_surface_get_width(ctx->surface);
    for (int i = 0; i < 6; i++)
    {
        if (first)
        {
            px = 5;
            py = 15;
        }
        else
            py += text.height + 3;

        if (text.width >= w || py >= h)
        {
            int pw = text.width >= w ? text.width + 20 : w;
            int ph = py >= h ? py + 20 : h;
            __gfx_audit_resize_cairo(ctx, pw, ph);
            /* Paint again after resizing, recursively */
            __gfx_audit_screen_render(ctx, gdi);
            break;
        }
        
        cairo_move_to(ctx->cairo, px, py);
        cairo_show_text(ctx->cairo, __text_paint_buffer[i]);

        cairo_text_extents(ctx->cairo, __text_paint_buffer[i], &text);
        first = FALSE;
    }
}

#define DUMP_DIR "/home/sora/Repositories/Hacking/FreeRDP/dump"
static void __gfx_audit_dump_argb_image(uint8_t *ptr, uint32_t w, uint32_t h, uint32_t stride)
{
    static uint32_t frame_id = 0;
    char path[512];
    snprintf(path, 512, DUMP_DIR"/snapshot_%d.rgba.raw", frame_id);

    int fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd < 0)
        return;
    
    uint32_t geometry[4] = { w, h, 0xffff2233, 0 };
    write(fd, (uint8_t *) geometry, sizeof(geometry));
    
    for (int i = 0; i < h; i++)
    {
        uint8_t *dat = ptr + stride * i;
        write(fd, dat, w * 4);
    }
    close(fd);
    frame_id++;
}

static void __gfx_audit_screen_paint(GFXAuditContext *ctx, gdiGfxSurface *surface)
{
    RdpgfxClientContext *gfx = (RdpgfxClientContext*) ctx->gfxContext;
    rdpGdi *gdi = (rdpGdi *) gfx->custom;

    uint8_t *pSrc = cairo_image_surface_get_data(ctx->surface);
    uint32_t nSrcStride = cairo_image_surface_get_stride(ctx->surface);
    uint8_t *pDst = gdi->primary_buffer;
    uint32_t nDstStride = gdi->stride;
    
    uint32_t nSrcWidth = cairo_image_surface_get_width(ctx->surface);
    uint32_t nSrcHeight = cairo_image_surface_get_height(ctx->surface);
    
    ctx->primitives->alphaComp_argb(
        pSrc, nSrcStride,
        surface->data, surface->scanline,
        pDst, nDstStride,
        nSrcWidth, nSrcHeight);
}

static void __gfx_audit_show_screen_fps(GFXAuditContext *ctx, gdiGfxSurface *surface, BOOL repaint)
{
    if (repaint)
    {
        __gfx_audit_screen_render(ctx, surface);
        uint8_t *ptr = cairo_image_surface_get_data(ctx->surface);
        uint32_t width = cairo_image_surface_get_width(ctx->surface);
        uint32_t height = cairo_image_surface_get_height(ctx->surface);
        uint32_t stride = cairo_image_surface_get_stride(ctx->surface);

        for (int i = 0; i < height; i++)
        {
            uint8_t *row = ptr + i * stride;
            for (int j = 0; j < width; j++)
                (row + j * 4)[3] = 190;
        }
    }
    const RECTANGLE_16 rect = {
        .top = 0,
        .left = 0,
        .bottom = cairo_image_surface_get_height(ctx->surface),
        .right = cairo_image_surface_get_width(ctx->surface)
    };
    region16_union_rect(&surface->invalidRegion, &surface->invalidRegion, &rect);
}

static void __gfx_audit_show_fps(GFXAuditContext *ctx, RdpgfxClientContext *gfx, gdiGfxSurface *surface)
{
    ftime(&ctx->fps_timer.et);
    struct timeb *st = &ctx->fps_timer.st;
    struct timeb *et = &ctx->fps_timer.et;
    double sec = (et->time - st->time) + (et->millitm - st->millitm) / 1e3;

    ctx->frames++;
    BOOL repaint = FALSE;
    if (sec >= 0.5)
    {
        ctx->fps = ctx->frames / sec;
        ftime(&ctx->fps_timer.st);
        ctx->frames = 0;
        repaint = TRUE;
    }

    if (gfx->showFpsOnConsole)
        __gfx_audit_show_console_fps(ctx, repaint);
    if (gfx->showFpsOnScreen)
        __gfx_audit_show_screen_fps(ctx, surface, repaint);
}

void GFXAuditAccept(GFXAuditContext *ctx, void *dstSurface)
{
    RdpgfxClientContext *gfx = (RdpgfxClientContext*) ctx->gfxContext;
    if (gfx->showFpsOnConsole || gfx->showFpsOnScreen)
        __gfx_audit_show_fps(ctx, gfx, (gdiGfxSurface*)dstSurface);
}

void GFXAuditPaintScreen(GFXAuditContext *ctx, void *surface)
{
    __gfx_audit_screen_paint(ctx, (gdiGfxSurface *)surface);
}

static void __gfx_audit_paint_horizontal_line(UINT32 *__ptr, int sx, int sy, int sw, int sh, int len, int height)
{
	for (int i = sy; i < height + sy; i++)
	{
		UINT32 *ptr = __ptr + sw * i;
		for (int j = sx; j < len + sx; j++)
		{
			if (i >= sh || j >= sw)
				continue;
            *(ptr + j) = 0xffb73c8a;
		}
	}
}

static void __gfx_audit_paint_vertical_line(UINT32 *__ptr, int sx, int sy, int sw, int sh, int len, int width)
{
	for (int i = sx; i < sx + width; i++)
	{
		UINT32 *ptr = __ptr + i;
		for (int j = sy; j < sy + len; j++)
		{
			if (i >= sw || j >= sh)
				continue;
			*(ptr + j * sw) = 0xffb73c8a;
		}
	}
}

void GFXAuditPaintUpdated(BYTE *pDst, const RECTANGLE_16 *regions, UINT32 num, UINT32 nDstWidth, UINT32 nDstHeight)
{
	for (int i = 0; i < num; i++)
	{
		const RECTANGLE_16 *area = &regions[i];
		const UINT32 height = area->bottom - area->top;
		const UINT32 width = area->right - area->left;
		__gfx_audit_paint_horizontal_line((UINT32*)pDst, area->left, area->top, nDstWidth, nDstHeight, width, 2);
		__gfx_audit_paint_horizontal_line((UINT32*)pDst, area->left, area->bottom - 2, nDstWidth, nDstHeight, width, 2);
		__gfx_audit_paint_vertical_line((UINT32*)pDst, area->left, area->top, nDstWidth, nDstHeight, height, 2);
		__gfx_audit_paint_vertical_line((UINT32*)pDst, area->right - 2, area->top, nDstWidth, nDstHeight, height, 2);
	}
}
