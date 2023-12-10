/*
 * Copyright (C) 2023 Apple, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(WEBXR_LAYERS)

#include "WebXRLayer.h"
#include "XRLayerLayout.h"
#include "XRLayerQuality.h"

#include <wtf/IsoMalloc.h>

namespace WebCore {

class XRCompositionLayer : public WebXRLayer {
    WTF_MAKE_ISO_ALLOCATED(XRCompositionLayer);
public:
    virtual ~XRCompositionLayer();

    XRLayerLayout layout() const { ASSERT_NOT_IMPLEMENTED_YET(); }

    bool blendTextureSourceAlpha() const { ASSERT_NOT_IMPLEMENTED_YET(); }
    [[noreturn]] void setBlendTextureSourceAlpha(bool) { ASSERT_NOT_IMPLEMENTED_YET(); }

    bool forceMonoPresentation() const { ASSERT_NOT_IMPLEMENTED_YET(); }
    [[noreturn]] void setForceMonoPresentation(bool) { ASSERT_NOT_IMPLEMENTED_YET(); }

    float opacity() const { ASSERT_NOT_IMPLEMENTED_YET(); }
    [[noreturn]] void setOpacity(float) { ASSERT_NOT_IMPLEMENTED_YET(); }

    uint32_t mipLevels() const { ASSERT_NOT_IMPLEMENTED_YET(); }

    XRLayerQuality quality() const { ASSERT_NOT_IMPLEMENTED_YET(); }
    [[noreturn]] void setQuality(XRLayerQuality) { ASSERT_NOT_IMPLEMENTED_YET(); }

    bool needsRedraw() const { ASSERT_NOT_IMPLEMENTED_YET(); }

    [[noreturn]] void destroy() { ASSERT_NOT_IMPLEMENTED_YET(); }
};

} // namespace WebCore

#endif // ENABLE(WEBXR_LAYERS)
