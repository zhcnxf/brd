/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSurface_DEFINED
#define SkSurface_DEFINED

#include "SkRefCnt.h"
#include "SkImage.h"
#include "SkSurfaceProps.h"

class SkCanvas;
class SkPaint;
class GrContext;
class GrRenderTarget;

/**
 *  SkSurface represents the backend/results of drawing to a canvas. For raster
 *  drawing, the surface will be pixels, but (for example) when drawing into
 *  a PDF or Picture canvas, the surface stores the recorded commands.
 *
 *  To draw into a canvas, first create the appropriate type of Surface, and
 *  then request the canvas from the surface.
 *
 *  SkSurface always has non-zero dimensions. If there is a request for a new surface, and either
 *  of the requested dimensions are zero, then NULL will be returned.
 */
class SK_API SkSurface : public SkRefCnt {
public:
    static const SkBudgeted kYes_Budgeted = SkBudgeted::kYes;
    static const SkBudgeted kNo_Budgeted = SkBudgeted::kNo;
    using Budgeted = SkBudgeted;
    
    /**
     *  Create a new surface, using the specified pixels/rowbytes as its
     *  backend.
     *
     *  If the requested surface cannot be created, or the request is not a
     *  supported configuration, NULL will be returned.
     */
    static SkSurface* NewRasterDirect(const SkImageInfo&, void* pixels, size_t rowBytes,
                                      const SkSurfaceProps* = NULL);

    /**
     *  The same as NewRasterDirect, but also accepts a call-back routine, which is invoked
     *  when the surface is deleted, and is passed the pixel memory and the specified context.
     */
    static SkSurface* NewRasterDirectReleaseProc(const SkImageInfo&, void* pixels, size_t rowBytes,
                                                 void (*releaseProc)(void* pixels, void* context),
                                                 void* context, const SkSurfaceProps* = NULL);

    /**
     *  Return a new surface, with the memory for the pixels automatically allocated, but respecting
     *  the specified rowBytes. If rowBytes==0, then a default value will be chosen. If a non-zero
     *  rowBytes is specified, then any images snapped off of this surface (via newImageSnapshot())
     *  are guaranteed to have the same rowBytes.
     *
     *  If the requested surface cannot be created, or the request is not a
     *  supported configuration, NULL will be returned.
     */
    static SkSurface* NewRaster(const SkImageInfo&, size_t rowBytes, const SkSurfaceProps*);

    /**
     *  Allocate a new surface, automatically computing the rowBytes.
     */
    static SkSurface* NewRaster(const SkImageInfo&, const SkSurfaceProps* = NULL);

    /**
     *  Helper version of NewRaster. It creates a SkImageInfo with the
     *  specified width and height, and populates the rest of info to match
     *  pixels in SkPMColor format.
     */
    static SkSurface* NewRasterN32Premul(int width, int height, const SkSurfaceProps* props = NULL) {
        return NewRaster(SkImageInfo::MakeN32Premul(width, height), props);
    }

    /**
     *  Return a new surface using the specified render target.
     */
    static SkSurface* NewRenderTargetDirect(GrRenderTarget*, const SkSurfaceProps*);

    static SkSurface* NewRenderTargetDirect(GrRenderTarget* target) {
        return NewRenderTargetDirect(target, NULL);
    }

    /**
     *  Used to wrap a pre-existing backend 3D API texture as a SkSurface. The kRenderTarget flag
     *  must be set on GrBackendTextureDesc for this to succeed. Skia will not assume ownership
     *  of the texture and the client must ensure the texture is valid for the lifetime of the
     *  SkSurface.
     */
    static SkSurface* NewFromBackendTexture(GrContext*, const GrBackendTextureDesc&,
                                            const SkSurfaceProps*);
    // Legacy alias
    static SkSurface* NewWrappedRenderTarget(GrContext* ctx, const GrBackendTextureDesc& desc,
                                             const SkSurfaceProps* props) {
        return NewFromBackendTexture(ctx, desc, props);
    }

    /**
     *  Used to wrap a pre-existing 3D API rendering target as a SkSurface. Skia will not assume
     *  ownership of the render target and the client must ensure the render target is valid for the
     *  lifetime of the SkSurface.
     */
    static SkSurface* NewFromBackendRenderTarget(GrContext*, const GrBackendRenderTargetDesc&,
                                                 const SkSurfaceProps*);

    /**
     *  Used to wrap a pre-existing 3D API texture as a SkSurface. Skia will treat the texture as
     *  a rendering target only, but unlike NewFromBackendRenderTarget, Skia will manage and own
     *  the associated render target objects (but not the provided texture). The kRenderTarget flag
     *  must be set on GrBackendTextureDesc for this to succeed. Skia will not assume ownership
     *  of the texture and the client must ensure the texture is valid for the lifetime of the
     *  SkSurface.
     */
    static SkSurface* NewFromBackendTextureAsRenderTarget(
            GrContext*, const GrBackendTextureDesc&, const SkSurfaceProps*);

    /**
     *  Return a new surface whose contents will be drawn to an offscreen
     *  render target, allocated by the surface.
     *
     *  The GrTextureStorageAllocator will be reused if SkImage snapshots create
     *  additional textures.
     */
    static SkSurface* NewRenderTarget(
            GrContext*, SkBudgeted, const SkImageInfo&, int sampleCount,
            const SkSurfaceProps* = NULL, GrTextureStorageAllocator = GrTextureStorageAllocator());

    static SkSurface* NewRenderTarget(GrContext* gr, SkBudgeted b, const SkImageInfo& info) {
        return NewRenderTarget(gr, b, info, 0);
    }

    int width() const { return fWidth; }
    int height() const { return fHeight; }

    /**
     *  Returns a unique non-zero, unique value identifying the content of this
     *  surface. Each time the content is changed changed, either by drawing
     *  into this surface, or explicitly calling notifyContentChanged()) this
     *  method will return a new value.
     *
     *  If this surface is empty (i.e. has a zero-dimention), this will return
     *  0.
     */
    uint32_t generationID();

    /**
     *  Modes that can be passed to notifyContentWillChange
     */
    enum ContentChangeMode {
        /**
         *  Use this mode if it is known that the upcoming content changes will
         *  clear or overwrite prior contents, thus making them discardable.
         */
        kDiscard_ContentChangeMode,
        /**
         *  Use this mode if prior surface contents need to be preserved or
         *  if in doubt.
         */
        kRetain_ContentChangeMode,
    };

    /**
     *  Call this if the contents are about to change. This will (lazily) force a new
     *  value to be returned from generationID() when it is called next.
     *
     *  CAN WE DEPRECATE THIS?
     */
    void notifyContentWillChange(ContentChangeMode mode);

    enum BackendHandleAccess {
        kFlushRead_BackendHandleAccess,     //!< caller may read from the backend object
        kFlushWrite_BackendHandleAccess,    //!< caller may write to the backend object
        kDiscardWrite_BackendHandleAccess,  //!< caller must over-write the entire backend object
    };

    /*
     * These are legacy aliases which will be removed soon
     */
    static const BackendHandleAccess kFlushRead_TextureHandleAccess =
            kFlushRead_BackendHandleAccess;
    static const BackendHandleAccess kFlushWrite_TextureHandleAccess =
            kFlushWrite_BackendHandleAccess;
    static const BackendHandleAccess kDiscardWrite_TextureHandleAccess =
            kDiscardWrite_BackendHandleAccess;


    /**
     *  Retrieves the backend API handle of the texture used by this surface, or 0 if the surface
     *  is not backed by a GPU texture.
     *
     *  The returned texture-handle is only valid until the next draw-call into the surface,
     *  or the surface is deleted.
     */
    GrBackendObject getTextureHandle(BackendHandleAccess);

    /**
     *  Retrieves the backend API handle of the RenderTarget backing this surface.  Callers must
     *  ensure this function returns 'true' or else the GrBackendObject will be invalid
     *
     *  In OpenGL this will return the FramebufferObject ID.
     */
    bool getRenderTargetHandle(GrBackendObject*, BackendHandleAccess);

    /**
     *  Return a canvas that will draw into this surface. This will always
     *  return the same canvas for a given surface, and is manged/owned by the
     *  surface. It should not be used when its parent surface has gone out of
     *  scope.
     */
    SkCanvas* getCanvas();

    /**
     *  Return a new surface that is "compatible" with this one, in that it will
     *  efficiently be able to be drawn into this surface. Typical calling
     *  pattern:
     *
     *  SkSurface* A = SkSurface::New...();
     *  SkCanvas* canvasA = surfaceA->newCanvas();
     *  ...
     *  SkSurface* surfaceB = surfaceA->newSurface(...);
     *  SkCanvas* canvasB = surfaceB->newCanvas();
     *  ... // draw using canvasB
     *  canvasA->drawSurface(surfaceB); // <--- this will always be optimal!
     */
    SkSurface* newSurface(const SkImageInfo&);

    /**
     *  Returns an image of the current state of the surface pixels up to this
     *  point. Subsequent changes to the surface (by drawing into its canvas)
     *  will not be reflected in this image. If a copy must be made the Budgeted
     *  parameter controls whether it counts against the resource budget
     *  (currently for the gpu backend only).
     */
    SkImage* newImageSnapshot(SkBudgeted = SkBudgeted::kYes);

    /**
     * In rare instances a client may want a unique copy of the SkSurface's contents in an image
     * snapshot. This enum can be used to enforce that the image snapshot's backing store is not
     * shared with another image snapshot or the surface's backing store. This is generally more
     * expensive. This was added for Chromium bug 585250.
     */
    enum ForceUnique {
        kNo_ForceUnique,
        kYes_ForceUnique
    };
    SkImage* newImageSnapshot(SkBudgeted, ForceUnique);

    /**
     *  Though the caller could get a snapshot image explicitly, and draw that,
     *  it seems that directly drawing a surface into another canvas might be
     *  a common pattern, and that we could possibly be more efficient, since
     *  we'd know that the "snapshot" need only live until we've handed it off
     *  to the canvas.
     */
    void draw(SkCanvas*, SkScalar x, SkScalar y, const SkPaint*);

    /**
     *  If the surface has direct access to its pixels (i.e. they are in local
     *  RAM) return the const-address of those pixels, and if not null, return
     *  the ImageInfo and rowBytes. The returned address is only valid while
     *  the surface object is in scope, and no API call is made on the surface
     *  or its canvas.
     *
     *  On failure, returns NULL and the info and rowBytes parameters are
     *  ignored.
     */
    const void* peekPixels(SkImageInfo* info, size_t* rowBytes);

    /**
     *  Copy the pixels from the surface into the specified buffer (pixels + rowBytes),
     *  converting them into the requested format (dstInfo). The surface pixels are read
     *  starting at the specified (srcX,srcY) location.
     *
     *  The specified ImageInfo and (srcX,srcY) offset specifies a source rectangle
     *
     *      srcR.setXYWH(srcX, srcY, dstInfo.width(), dstInfo.height());
     *
     *  srcR is intersected with the bounds of the base-layer. If this intersection is not empty,
     *  then we have two sets of pixels (of equal size). Replace the dst pixels with the
     *  corresponding src pixels, performing any colortype/alphatype transformations needed
     *  (in the case where the src and dst have different colortypes or alphatypes).
     *
     *  This call can fail, returning false, for several reasons:
     *  - If srcR does not intersect the surface bounds.
     *  - If the requested colortype/alphatype cannot be converted from the surface's types.
     */
    bool readPixels(const SkImageInfo& dstInfo, void* dstPixels, size_t dstRowBytes,
                    int srcX, int srcY);

    const SkSurfaceProps& props() const { return fProps; }

    /**
     * Issue any pending surface IO to the current backend 3D API and resolve any surface MSAA.
     */
    void prepareForExternalIO();

protected:
    SkSurface(int width, int height, const SkSurfaceProps*);
    SkSurface(const SkImageInfo&, const SkSurfaceProps*);

    // called by subclass if their contents have changed
    void dirtyGenerationID() {
        fGenerationID = 0;
    }

private:
    const SkSurfaceProps fProps;
    const int            fWidth;
    const int            fHeight;
    uint32_t             fGenerationID;

    typedef SkRefCnt INHERITED;
};

#endif
