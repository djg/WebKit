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

#if ENABLE(WEBXR_HIT_TEST)

#include <JavaScriptCore/Forward.h>
#include <wtf/IsoMalloc.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class DOMPointReadOnly;
class WebXRRigidTransform;

struct DOMPointInit;

// https://immersive-web.github.io/hit-test/#xrray-interface
class XRRay : public RefCounted<XRRay>
{
    WTF_MAKE_ISO_ALLOCATED(XRRay);
public:
    // https://immersive-web.github.io/hit-test/#xr-ray-direction-init-dictionary
    struct DirectionInit {
        double x;
        double y;
        double z;
        double w;
    };

    static Ref<XRRay> create(const DOMPointInit&, const DirectionInit&);
    static Ref<XRRay> create(WebXRRigidTransform&);

    const DOMPointReadOnly& origin() const;
    const DOMPointReadOnly& direction() const;
    const Float32Array& matrix();
};

} // namespace WebCore

#endif
