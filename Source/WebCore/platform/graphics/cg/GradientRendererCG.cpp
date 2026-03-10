/*
 * Copyright (C) 2021-2025 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "GradientRendererCG.h"

#include "ColorHash.h"
#include "ColorInterpolation.h"
#include "ColorSpaceCG.h"
#include "GradientColorStops.h"
#include <pal/spi/cg/CoreGraphicsSPI.h>
#include <wtf/HashMap.h>
#include <wtf/TinyLRUCache.h>

namespace WTF {
using namespace WebCore;

struct SampledGradientCacheKey {
    ColorInterpolationMethod interpolationMethod;
    GradientColorStops::StopVector colorStops;

    friend bool operator==(const SampledGradientCacheKey&, const SampledGradientCacheKey&) = default;
};

template<>
bool TinyLRUCachePolicy<SampledGradientCacheKey, RetainPtr<CGGradientRef>>::isKeyNull(const SampledGradientCacheKey& key)
{
    return key.colorStops.isEmpty();
}

template<>
RetainPtr<CGGradientRef> TinyLRUCachePolicy<SampledGradientCacheKey, RetainPtr<CGGradientRef>>::createValueForKey(const SampledGradientCacheKey& params)
{
    return WebCore::GradientRendererCG::createGradientBySampling(params.interpolationMethod, params.colorStops);
}

} // namespace WTF

namespace WebCore {


GradientRendererCG::GradientRendererCG(ColorInterpolationMethod colorInterpolationMethod, const GradientColorStops& stops)
    : m_gradient { pickStrategy(colorInterpolationMethod, stops) }
{
}

// MARK: - Strategy selection.

static bool anyComponentIsNone(const GradientColorStops& stops)
{
    for (auto& stop : stops) {
        if (stop.color.anyComponentIsNone())
            return true;
    }
    
    return false;
}

GradientRendererCG::Gradient GradientRendererCG::pickStrategy(ColorInterpolationMethod colorInterpolationMethod, const GradientColorStops& stops) const
{
    return WTF::switchOn(colorInterpolationMethod.colorSpace,
        [&] (const ColorInterpolationMethod::SRGB&) -> Gradient {
            // FIXME: As an optimization we can precompute 'none' replacements and create a transformed stop list rather than falling back on gradient sampling.
            if (anyComponentIsNone(stops))
                return makeGradientBySampling(colorInterpolationMethod, stops);

            return makeGradient(colorInterpolationMethod, stops);
        },
        [&] (const auto&) -> Gradient {
            return makeGradientBySampling(colorInterpolationMethod, stops);
        }
    );
}

// MARK: - Gradient strategy.

GradientRendererCG::Gradient GradientRendererCG::makeGradient(ColorInterpolationMethod colorInterpolationMethod, const GradientColorStops& stops) const
{
    ASSERT_UNUSED(colorInterpolationMethod, std::holds_alternative<ColorInterpolationMethod::SRGB>(colorInterpolationMethod.colorSpace));

    auto gradientInterpolatesPremultipliedOptionsDictionary = [] () -> CFDictionaryRef {
        static CFTypeRef keys[] = { kCGGradientInterpolatesPremultiplied };
        static CFTypeRef values[] = { kCFBooleanTrue };
        static CFDictionaryRef options = CFDictionaryCreate(kCFAllocatorDefault, keys, values, std::size(keys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

        return options;
    };

   auto gradientOptionsDictionary = [&] (auto colorInterpolationMethod) -> CFDictionaryRef {
        switch (colorInterpolationMethod.alphaPremultiplication) {
        case AlphaPremultiplication::Unpremultiplied:
            return nullptr;
        case AlphaPremultiplication::Premultiplied:
            return gradientInterpolatesPremultipliedOptionsDictionary();
        }
   };

    auto hasOnlyBoundedSRGBColorStops = [] (const auto& stops) {
        for (const auto& stop : stops) {
            if (stop.color.colorSpace() != ColorSpace::SRGB)
                return false;
        }
        return true;
    };

    auto numberOfStops = stops.size();

    static constexpr auto reservedStops = 3;
    Vector<CGFloat, reservedStops> locations;
    locations.reserveInitialCapacity(numberOfStops);

    Vector<CGFloat, 4 * reservedStops> colorComponents;
    colorComponents.reserveInitialCapacity(numberOfStops * 4);

    RetainPtr cgColorSpace = [&] {
        // FIXME: Now that we only ever use CGGradientCreateWithColorComponents, we should investigate
        // if there is any real benefit to using sRGB when all the stops are bounded vs just using
        // extended sRGB for all gradients.
        if (hasOnlyBoundedSRGBColorStops(stops)) {
            for (const auto& stop : stops) {
                auto [r, g, b, a] = stop.color.toColorTypeLossy<SRGBA<float>>().resolved();
                colorComponents.appendList({ r, g, b, a });

                locations.append(stop.offset);
            }

            return cachedCGColorSpaceSingleton<ColorSpaceFor<SRGBA<float>>>();
        }

        using OutputSpaceColorType = std::conditional_t<HasCGColorSpaceMapping<ColorSpace::ExtendedSRGB>, ExtendedSRGBA<float>, SRGBA<float>>;
        for (const auto& stop : stops) {
            auto [r, g, b, a] = stop.color.toColorTypeLossy<OutputSpaceColorType>().resolved();
            colorComponents.appendList({ r, g, b, a });

            locations.append(stop.offset);
        }
        return cachedCGColorSpaceSingleton<ColorSpaceFor<OutputSpaceColorType>>();
    }();

    // CoreGraphics has a bug where if the last two stops are at 1, it fails to extend the last stop's color. This can be visible in radial gradients.
    auto apply139572277Workaround = [&]() {
        if (numberOfStops < 2)
            return;

        if (locations[numberOfStops - 2] == 1.0 && locations[numberOfStops - 1] == 1.0) {
            // Replicate the last color stop.
            locations.append(1.0);

            auto lastColorComponentIndex = 4 * (numberOfStops - 1);

            colorComponents.reserveCapacity((numberOfStops + 1) * 4);
            colorComponents.append(colorComponents[lastColorComponentIndex]);
            colorComponents.append(colorComponents[lastColorComponentIndex + 1]);
            colorComponents.append(colorComponents[lastColorComponentIndex + 2]);
            colorComponents.append(colorComponents[lastColorComponentIndex + 3]);

            ++numberOfStops;
        }
    };

    apply139572277Workaround();

    return Gradient { adoptCF(CGGradientCreateWithColorComponentsAndOptions(cgColorSpace.get(), colorComponents.span().data(), locations.span().data(), numberOfStops, gradientOptionsDictionary(colorInterpolationMethod))) };
}

// MARK: - Shading strategy.

template<typename InterpolationSpace, AlphaPremultiplication alphaPremultiplication>
void GradientRendererCG::Shading::shadingFunction(void* info, const CGFloat* rawIn, CGFloat* rawOut)
{
    using InterpolationSpaceColorType = typename InterpolationSpace::ColorType;
    using OutputSpaceColorType = std::conditional_t<HasCGColorSpaceMapping<ColorSpace::ExtendedSRGB>, ExtendedSRGBA<float>, SRGBA<float>>;

    auto* data = static_cast<GradientRendererCG::Shading::Data*>(info);

    // Compute color at offset 'in[0]' and assign the components to out[0 -> 3].
    auto in = unsafeMakeSpan(rawIn, 1);
    auto out = unsafeMakeSpan(rawOut, 4);

    float requestedOffset = in[0];

    // 1. Find stops that bound the requested offset.
    auto [stop0, stop1] = [&] {
        for (size_t stop = 1; stop < data->stops().size(); ++stop) {
            if (requestedOffset <= data->stops()[stop].offset)
                return std::tie(data->stops()[stop - 1], data->stops()[stop]);
        }
        RELEASE_ASSERT_NOT_REACHED();
    }();

    // 2. Compute percentage offset between the two stops.
    float offset = (stop1.offset == stop0.offset) ? 0.0f : (requestedOffset - stop0.offset) / (stop1.offset - stop0.offset);

    // 3. Interpolate the two stops' colors by the computed offset.
    // Synthetic color stops are added to extend the author-provided gradient out to 0 and 1
    // with a solid color, if necessary. These need special handling because `longer hue` gradients
    // would otherwise rotate through 360° of hue in these segments.
    auto interpolatedColor = [&]() {
        if (stop0.offset == 0.0f && data->firstStopIsSynthetic())
            return makeFromComponents<InterpolationSpaceColorType>(stop0.colorComponents);

        if (stop1.offset == 1.0f && data->lastStopIsSynthetic())
            return makeFromComponents<InterpolationSpaceColorType>(stop1.colorComponents);

        return interpolateColorComponents<alphaPremultiplication>(
            std::get<InterpolationSpace>(data->colorInterpolationMethod().colorSpace),
            makeFromComponents<InterpolationSpaceColorType>(stop0.colorComponents), 1.0f - offset,
            makeFromComponents<InterpolationSpaceColorType>(stop1.colorComponents), offset);
    }();

    // 4. Convert to the output color space.
    auto interpolatedColorConvertedToOutputSpace = asColorComponents(convertColor<OutputSpaceColorType>(interpolatedColor).resolved());

    // 5. Write color components to 'out' pointer.
    for (size_t componentIndex = 0; componentIndex < interpolatedColorConvertedToOutputSpace.size(); ++componentIndex)
        out[componentIndex] = interpolatedColorConvertedToOutputSpace[componentIndex];
}

// MARK: - Gradient-by-sampling strategy.

static ColorComponents<float, 4> evaluateGradientAtOffset(CGFunctionEvaluateCallback evaluate, void* info, float offset)
{
    CGFloat in = offset;
    CGFloat out[4] = { };
    evaluate(info, &in, out);
    return ColorComponents<float, 4>(static_cast<float>(out[0]), static_cast<float>(out[1]), static_cast<float>(out[2]), static_cast<float>(out[3]));
}

static void bisectAndCollectGradientStops(
    CGFunctionEvaluateCallback evaluate, void* info,
    float offset0, ColorComponents<float, 4> color0,
    float offset1, ColorComponents<float, 4> color1,
    Vector<CGFloat>& locations, Vector<CGFloat>& components)
{
    // Stop recursing when the segment is narrower than one step of a 2048-wide LUT.
    static constexpr float minimumSegmentWidth = 1.0f / 2048.0f;
    if (offset1 - offset0 < minimumSegmentWidth)
        return;

    float midOffset = (offset0 + offset1) * 0.5f;
    auto colorMid = evaluateGradientAtOffset(evaluate, info, midOffset);

    // Compute the linearly-interpolated color at the midpoint and measure the error.
    auto colorLerp = mapColorComponents([](float c0, float c1) { return c0 + 0.5f * (c1 - c0); }, color0, color1); // NOLINT
    auto absDiff = mapColorComponents([](float a, float b) { return std::abs(a - b); }, colorMid, colorLerp); // NOLINT
    float maxDiff = std::max({ absDiff[0], absDiff[1], absDiff[2], absDiff[3] });

    static constexpr float tolerance = 8.0f / 255.0f;
    if (maxDiff <= tolerance)
        return;

    // The segment is not locally linear: recurse into both halves, then insert a stop
    // at the midpoint so CGGradient interpolates through the correct color.
    bisectAndCollectGradientStops(evaluate, info, offset0, color0, midOffset, colorMid, locations, components);

    locations.append(midOffset);
    for (size_t i = 0; i < 4; ++i)
        components.append(colorMid[i]);

    bisectAndCollectGradientStops(evaluate, info, midOffset, colorMid, offset1, color1, locations, components);
}

// Pre-sample non-sRGB color stops into a set of ExtendedSRGB CGGradient stops,
// avoiding the per-pixel CGShading callback overhead at draw time.
GradientRendererCG::Gradient GradientRendererCG::makeGradientBySampling(ColorInterpolationMethod colorInterpolationMethod, const GradientColorStops& stops) const
{
    auto colorStops = stops.sorted().stops();
    static NeverDestroyed<TinyLRUCache<WTF::SampledGradientCacheKey, RetainPtr<CGGradientRef>, 8>> cache;
    RetainPtr gradient = cache.get().get({ colorInterpolationMethod, colorStops });
    return Gradient { WTF::move(gradient) };
}

RetainPtr<CGGradientRef> GradientRendererCG::createGradientBySampling(ColorInterpolationMethod colorInterpolationMethod, const GradientColorStops::StopVector& stops)
{
    using OutputSpaceColorType = std::conditional_t<HasCGColorSpaceMapping<ColorSpace::ExtendedSRGB>, ExtendedSRGBA<float>, SRGBA<float>>;

    // Build shading data with stops converted to the interpolation color space.
    auto convertColorToColorInterpolationSpace = [&](const Color& color) -> ColorComponents<float, 4> {
        return WTF::switchOn(colorInterpolationMethod.colorSpace,
            [&]<typename MethodColorSpace>(const MethodColorSpace&) -> ColorComponents<float, 4> {
                using ColorType = typename MethodColorSpace::ColorType;
                return asColorComponents(color.template toColorTypeLossyCarryingForwardMissing<ColorType>().unresolved());
            });
    };

    auto totalNumberOfStops = stops.size();
    bool hasZero = false;
    bool hasOne = false;

    for (const auto& stop : stops) {
        if (stop.offset == 0) // NOLINT
            hasZero = true;
        else if (stop.offset == 1)
            hasOne = true;
    }

    if (!hasZero)
        totalNumberOfStops++;
    if (!hasOne)
        totalNumberOfStops++;

    Vector<ColorConvertedToInterpolationColorSpaceStop> convertedStops;
    convertedStops.reserveInitialCapacity(totalNumberOfStops);

    if (!hasZero)
        convertedStops.append({ 0.0f, { 0.0f, 0.0f, 0.0f, 0.0f } });

    convertedStops.appendContainerWithMapping(stops, [&](const auto& stop) {
        return ColorConvertedToInterpolationColorSpaceStop { stop.offset, convertColorToColorInterpolationSpace(stop.color) };
    });

    if (!hasOne)
        convertedStops.append({ 1.0f, convertedStops.last().colorComponents });

    if (!hasZero)
        convertedStops[0].colorComponents = convertedStops[1].colorComponents;

    auto data = GradientRendererCG::Shading::Data::create(colorInterpolationMethod, WTF::move(convertedStops), !hasZero, !hasOne);

    auto evaluate = WTF::switchOn(colorInterpolationMethod.colorSpace,
        [&]<typename MethodColorSpace>(const MethodColorSpace&) -> CGFunctionEvaluateCallback {
            switch (colorInterpolationMethod.alphaPremultiplication) {
            case AlphaPremultiplication::Unpremultiplied:
                return &GradientRendererCG::Shading::shadingFunction<MethodColorSpace, AlphaPremultiplication::Unpremultiplied>;
            case AlphaPremultiplication::Premultiplied:
                return &GradientRendererCG::Shading::shadingFunction<MethodColorSpace, AlphaPremultiplication::Premultiplied>;
            }
        });
    void* info = data.ptr();

    // Sample the gradient at coarse intervals using a prime-number step size
    // then adaptively bisect each interval to resolve non-linear color
    // transitions.
    static constexpr int sampleInterval = 19;

    struct CoarseSample {
        float offset;
        ColorComponents<float, 4> color;
    };

    Vector<CoarseSample> coarseSamples;
    coarseSamples.reserveInitialCapacity(sampleInterval + 1);

    for (int i = 0; i <= sampleInterval; ++i) {
        float offset = static_cast<float>(i) / sampleInterval;
        coarseSamples.append({ offset, evaluateGradientAtOffset(evaluate, info, offset) });
    }

    Vector<CGFloat> locations;
    Vector<CGFloat> components;

    // Append the first coarse sample.
    locations.append(coarseSamples[0].offset);
    for (size_t i = 0; i < 4; ++i)
        components.append(coarseSamples[0].color[i]);

    // For each coarse interval, adaptively bisect to find non-linear sub-segments,
    // then append the right endpoint of the interval.
    for (int i = 0; i < sampleInterval; ++i) {
        bisectAndCollectGradientStops(evaluate, info,
            coarseSamples[i].offset, coarseSamples[i].color,
            coarseSamples[i + 1].offset, coarseSamples[i + 1].color,
            locations, components);

        locations.append(coarseSamples[i + 1].offset);
        for (size_t j = 0; j < 4; ++j)
            components.append(coarseSamples[i + 1].color[j]);
    }

    ASSERT(locations.size() * 4 == components.size());

    auto cgColorSpace = cachedCGColorSpaceSingleton<ColorSpaceFor<OutputSpaceColorType>>();
    return adoptCF(CGGradientCreateWithColorComponentsAndOptions(cgColorSpace,
        components.span().data(), locations.span().data(), locations.size(), nullptr));
}

// MARK: - Drawing functions.

void GradientRendererCG::drawLinearGradient(CGContextRef platformContext, CGPoint startPoint, CGPoint endPoint, CGGradientDrawingOptions options)
{
    CGContextDrawLinearGradient(platformContext, m_gradient.get(), startPoint, endPoint, options);
}

void GradientRendererCG::drawRadialGradient(CGContextRef platformContext, CGPoint startCenter, CGFloat startRadius, CGPoint endCenter, CGFloat endRadius, CGGradientDrawingOptions options)
{
    CGContextDrawRadialGradient(platformContext, m_gradient.get(), startCenter, startRadius, endCenter, endRadius, options);
}

void GradientRendererCG::drawConicGradient(CGContextRef platformContext, CGPoint center, CGFloat angle)
{
    CGContextDrawConicGradient(platformContext, m_gradient.get(), center, angle);
}

}
