/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrTextureMaker_DEFINED
#define GrTextureMaker_DEFINED

#include "GrSamplerParams.h"
#include "GrResourceKey.h"
#include "GrTexture.h"
#include "SkTLazy.h"

class GrContext;
class GrSamplerParams;
class GrUniqueKey;
class SkBitmap;

/**
 * Different GPUs and API extensions have different requirements with respect to what texture
 * sampling parameters may be used with textures of various types. This class facilitates making
 * texture compatible with a given GrSamplerParams. There are two immediate subclasses defined
 * below. One is a base class for sources that are inherently texture-backed (e.g. a texture-backed
 * SkImage). It supports subsetting the original texture. The other is for use cases where the
 * source can generate a texture that represents some content (e.g. cpu pixels, SkPicture, ...).
 */
class GrTextureProducer : public SkNoncopyable {
public:
    struct CopyParams {
        GrSamplerParams::FilterMode fFilter;
        int                         fWidth;
        int                         fHeight;
    };

    enum FilterConstraint {
        kYes_FilterConstraint,
        kNo_FilterConstraint,
    };

    /**
     * Helper for creating a fragment processor to sample the texture with a given filtering mode.
     * It attempts to avoid making texture copies or using domains whenever possible.
     *
     * @param textureMatrix                    Matrix used to access the texture. It is applied to
     *                                         the local coords. The post-transformed coords should
     *                                         be in texel units (rather than normalized) with
     *                                         respect to this Producer's bounds (width()/height()).
     * @param constraintRect                   A rect that represents the area of the texture to be
     *                                         sampled. It must be contained in the Producer's bounds
     *                                         as defined by width()/height().
     * @param filterConstriant                 Indicates whether filtering is limited to
     *                                         constraintRect.
     * @param coordsLimitedToConstraintRect    Is it known that textureMatrix*localCoords is bound
     *                                         by the portion of the texture indicated by
     *                                         constraintRect (without consideration of filter
     *                                         width, just the raw coords).
     * @param filterOrNullForBicubic           If non-null indicates the filter mode. If null means
     *                                         use bicubic filtering.
     **/
    virtual sk_sp<GrFragmentProcessor> createFragmentProcessor(
                                    const SkMatrix& textureMatrix,
                                    const SkRect& constraintRect,
                                    FilterConstraint filterConstraint,
                                    bool coordsLimitedToConstraintRect,
                                    const GrSamplerParams::FilterMode* filterOrNullForBicubic,
                                    SkColorSpace* dstColorSpace,
                                    SkDestinationSurfaceColorMode) = 0;

    virtual ~GrTextureProducer() {}

    int width() const { return fWidth; }
    int height() const { return fHeight; }
    bool isAlphaOnly() const { return fIsAlphaOnly; }
    virtual SkAlphaType alphaType() const = 0;

protected:
    GrTextureProducer(int width, int height, bool isAlphaOnly)
        : fWidth(width)
        , fHeight(height)
        , fIsAlphaOnly(isAlphaOnly) {}

    /** Helper for creating a key for a copy from an original key. */
    static void MakeCopyKeyFromOrigKey(const GrUniqueKey& origKey,
                                       const CopyParams& copyParams,
                                       GrUniqueKey* copyKey) {
        SkASSERT(!copyKey->isValid());
        if (origKey.isValid()) {
            static const GrUniqueKey::Domain kDomain = GrUniqueKey::GenerateDomain();
            GrUniqueKey::Builder builder(copyKey, origKey, kDomain, 3);
            builder[0] = copyParams.fFilter;
            builder[1] = copyParams.fWidth;
            builder[2] = copyParams.fHeight;
        }
    }

    /**
    *  If we need to make a copy in order to be compatible with GrTextureParams producer is asked to
    *  return a key that identifies its original content + the CopyParms parameter. If the producer
    *  does not want to cache the stretched version (e.g. the producer is volatile), this should
    *  simply return without initializing the copyKey. If the texture generated by this producer
    *  depends on colorMode, then that information should also be incorporated in the key.
    */
    virtual void makeCopyKey(const CopyParams&, GrUniqueKey* copyKey,
                             SkDestinationSurfaceColorMode colorMode) = 0;

    /**
    *  If a stretched version of the texture is generated, it may be cached (assuming that
    *  makeCopyKey() returns true). In that case, the maker is notified in case it
    *  wants to note that for when the maker is destroyed.
    */
    virtual void didCacheCopy(const GrUniqueKey& copyKey) = 0;

private:
    const int   fWidth;
    const int   fHeight;
    const bool  fIsAlphaOnly;

    typedef SkNoncopyable INHERITED;
};

/**
 * Base class for sources that start out as textures. Optionally allows for a content area subrect.
 * The intent is not to use content area for subrect rendering. Rather, the pixels outside the
 * content area have undefined values and shouldn't be read *regardless* of filtering mode or
 * the SkCanvas::SrcRectConstraint used for subrect draws.
 */
class GrTextureAdjuster : public GrTextureProducer {
public:
    /** Makes the subset of the texture safe to use with the given texture parameters.
        outOffset will be the top-left corner of the subset if a copy is not made. Otherwise,
        the copy will be tight to the contents and outOffset will be (0, 0). If the copy's size
        does not match subset's dimensions then the contents are scaled to fit the copy.*/
    GrTexture* refTextureSafeForParams(const GrSamplerParams&, SkDestinationSurfaceColorMode,
                                       SkIPoint* outOffset);

    sk_sp<GrFragmentProcessor> createFragmentProcessor(
                                const SkMatrix& textureMatrix,
                                const SkRect& constraintRect,
                                FilterConstraint,
                                bool coordsLimitedToConstraintRect,
                                const GrSamplerParams::FilterMode* filterOrNullForBicubic,
                                SkColorSpace* dstColorSpace,
                                SkDestinationSurfaceColorMode) override;

    // We do not ref the texture nor the colorspace, so the caller must keep them in scope while
    // this Adjuster is alive.
    GrTextureAdjuster(GrTexture*, SkAlphaType, const SkIRect& area, uint32_t uniqueID,
                      SkColorSpace*);

protected:
    SkAlphaType alphaType() const override { return fAlphaType; }
    void makeCopyKey(const CopyParams& params, GrUniqueKey* copyKey,
                     SkDestinationSurfaceColorMode colorMode) override;
    void didCacheCopy(const GrUniqueKey& copyKey) override;

    GrTexture* originalTexture() const { return fOriginal; }

    /** Returns the content area or null for the whole original texture */
    const SkIRect* contentAreaOrNull() { return fContentArea.getMaybeNull(); }

private:
    SkTLazy<SkIRect>    fContentArea;
    GrTexture*          fOriginal;
    SkAlphaType         fAlphaType;
    SkColorSpace*       fColorSpace;
    uint32_t            fUniqueID;

    GrTexture* refCopy(const CopyParams &copyParams);

    typedef GrTextureProducer INHERITED;
};

/**
 * Base class for sources that start out as something other than a texture (encoded image,
 * picture, ...).
 */
class GrTextureMaker : public GrTextureProducer {
public:
    /**
     *  Returns a texture that is safe for use with the params. If the size of the returned texture
     *  does not match width()/height() then the contents of the original must be scaled to fit
     *  the texture. Places the color space of the texture in (*texColorSpace).
     */
    GrTexture* refTextureForParams(const GrSamplerParams&, SkDestinationSurfaceColorMode,
                                   sk_sp<SkColorSpace>* texColorSpace);

    sk_sp<GrFragmentProcessor> createFragmentProcessor(
                                const SkMatrix& textureMatrix,
                                const SkRect& constraintRect,
                                FilterConstraint filterConstraint,
                                bool coordsLimitedToConstraintRect,
                                const GrSamplerParams::FilterMode* filterOrNullForBicubic,
                                SkColorSpace* dstColorSpace,
                                SkDestinationSurfaceColorMode) override;

protected:
    GrTextureMaker(GrContext* context, int width, int height, bool isAlphaOnly)
        : INHERITED(width, height, isAlphaOnly)
        , fContext(context) {}

    /**
     *  Return the maker's "original" texture. It is the responsibility of the maker to handle any
     *  caching of the original if desired.
     */
    virtual GrTexture* refOriginalTexture(bool willBeMipped, SkDestinationSurfaceColorMode) = 0;

    /**
     *  Returns the color space of the maker's "original" texture, assuming it was retrieved with
     *  the same destination color mode.
     */
    virtual sk_sp<SkColorSpace> getColorSpace(SkDestinationSurfaceColorMode) = 0;

    /**
     *  Return a new (uncached) texture that is the stretch of the maker's original.
     *
     *  The base-class handles general logic for this, and only needs access to the following
     *  method:
     *  - refOriginalTexture()
     *
     *  Subclass may override this if they can handle creating the texture more directly than
     *  by copying.
     */
    virtual GrTexture* generateTextureForParams(const CopyParams&, bool willBeMipped,
                                                SkDestinationSurfaceColorMode);

    GrContext* context() const { return fContext; }

private:
    GrContext*  fContext;

    typedef GrTextureProducer INHERITED;
};

#endif
