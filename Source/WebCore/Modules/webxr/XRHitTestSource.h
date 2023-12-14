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

#include "TransformationMatrix.h"

#include <wtf/IsoMalloc.h>
#include <wtf/Ref.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/WeakPtr.h>

namespace WebCore {

class WebXRSession;
class XRRay;

enum class XRHitTestTrackableType : uint8_t;

// https://immersive-web.github.io/hit-test/#hit-test-source-interface
class XRHitTestSource : public RefCounted<XRHitTestSource>, public CanMakeWeakPtr<XRHitTestSource> {
    WTF_MAKE_ISO_ALLOCATED(XRHitTestSource);
public:
    static Ref<XRHitTestSource> create(WebXRSession& session, const WebXRSpace& space, Vector<XRHitTestTrackableType>&& entityTypes, XRRay&);
    ExceptionOr<void> cancel();

protected:
    XRHitTestSource(WebXRSession&, std::optional<TransformationMatrix>&&, Vector<XRHitTestTrackableType>&&, XRRay&);

private:
    Ref<WebXRSession> m_session;
    std::optional<TransformationMatrix> m_nativeOrigin;
    Vector<XRHitTestTrackableType> m_entityTypes;
    Ref<XRRay> m_offsetRay;
};

} // namespace WebCore

#endif
