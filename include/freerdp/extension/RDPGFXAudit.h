#ifndef __RDPGFX_AUDIT_H__
#define __RDPGFX_AUDIT_H__

#include <cairo/cairo.h>
#include <sys/timeb.h>
#include <freerdp/types.h>
#include <freerdp/primitives.h>

/**
 * This extension can print some performance information
 * in standard output, or render them on screen (Need Cairo
 * library).
*/

#define CAIRO_AUDIT_SURFACE_W	200
#define CAIRO_AUDIT_SURFACE_H 	50

typedef struct _GFXAuditContext
{
    struct
    {
        struct timeb st;
        struct timeb et;
        /* Delta-T in milliseconds */
        unsigned long dt;
    } decode_timer, frame_timer, fps_timer;
    char const *decoderName;
    cairo_t *cairo;
    cairo_surface_t *surface;
    /* This should be RdpgfxClientContext type */
    void *gfxContext;
    uint64_t frames;
    double fps;
    primitives_t *primitives;
} GFXAuditContext;

GFXAuditContext *GFXAuditNewContext(void *gfxClientContext);
void GFXAuditReleaseContext(GFXAuditContext *ctx);
void GFXAuditSetDecoderName(GFXAuditContext *ctx, char const *name);
void GFXAuditDecodeStartTimer(GFXAuditContext *ctx);
void GFXAuditDecodeStopTimer(GFXAuditContext *ctx);
void GFXAuditFrameStartTimer(GFXAuditContext *ctx);
void GFXAuditFrameStopTimer(GFXAuditContext *ctx);
/**
 * @param dstSurface should be gdiGfxSurface type that defined in rdpgfx.h.
 */
void GFXAuditAccept(GFXAuditContext *ctx, void *dstSurface);
/**
 * @param surface: Should be 'gdiGfxSurface' type.
 */
void GFXAuditPaintScreen(GFXAuditContext *ctx, void *surface);

void GFXAuditPaintUpdated(BYTE *pDst, const RECTANGLE_16 *regions, UINT32 num, UINT32 nDstWidth, UINT32 nDstHeight);
#endif /* __RDPGFX_AUDIT_H__ */
