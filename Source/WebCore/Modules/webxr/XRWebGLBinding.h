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

#include "XREye.h"

#include <ExceptionOr.h>
#include <wtf/IsoMalloc.h>
#include <wtf/RefPtr.h>
#include <wtf/Ref.h>

namespace WebCore {

class WebGL2RenderingContext;
class WebGLRenderingContext;
class WebXRFrame;
class WebXRSession;
class WebXRView;
class XRCompositionLayer;
class XRCubeLayer;
class XRCylinderLayer;
class XREquirectLayer;
class XRProjectionLayer;
class XRQuadLayer;
class XRWebGLSubImage;

struct XRCubeLayerInit;
struct XRCylinderLayerInit;
struct XREquirectLayerInit;
struct XRProjectionLayerInit;
struct XRQuadLayerInit;

// https://immersive-web.github.io/layers/#XRWebGLBindingtype
class XRWebGLBinding : public RefCounted<XRWebGLBinding> {
    WTF_MAKE_ISO_ALLOCATED(XRWebGLBinding);
public:

    using WebXRWebGLRenderingContext = std::variant<
        RefPtr<WebGLRenderingContext>,
        RefPtr<WebGL2RenderingContext>
    >;

    static ExceptionOr<Ref<XRWebGLBinding>> create(Ref<WebXRSession>&&, WebXRWebGLRenderingContext&&);
    ~XRWebGLBinding() = default;

    double nativeProjectionScaleFactor() const { ASSERT_NOT_IMPLEMENTED_YET(); }
    bool usesDepthValues() const { ASSERT_NOT_IMPLEMENTED_YET(); }

    ExceptionOr<Ref<XRProjectionLayer>> createProjectionLayer(const XRProjectionLayerInit&) { ASSERT_NOT_IMPLEMENTED_YET(); }
    ExceptionOr<Ref<XRQuadLayer>> createQuadLayer(const XRQuadLayerInit&) { ASSERT_NOT_IMPLEMENTED_YET(); }
    ExceptionOr<Ref<XRCylinderLayer>> createCylinderLayer(const XRCylinderLayerInit&) { ASSERT_NOT_IMPLEMENTED_YET(); }
    ExceptionOr<Ref<XREquirectLayer>> createEquirectLayer(const XREquirectLayerInit&) { ASSERT_NOT_IMPLEMENTED_YET(); }
    ExceptionOr<Ref<XRCubeLayer>> createCubeLayer(const XRCubeLayerInit&) { ASSERT_NOT_IMPLEMENTED_YET(); }

    ExceptionOr<Ref<XRWebGLSubImage>> getSubImage(const XRCompositionLayer&, const WebXRFrame&, XREye) { ASSERT_NOT_IMPLEMENTED_YET(); }
    ExceptionOr<Ref<XRWebGLSubImage>> getViewSubImage(const XRProjectionLayer&, const WebXRView&) { ASSERT_NOT_IMPLEMENTED_YET(); }
};

} // namespace WebCore

#endif // ENABLE(WEBXR_LAYERS)
