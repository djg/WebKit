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

#include <wtf/IsoMalloc.h>
#include <wtf/Ref.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class WebXRFrame;
class WebXRInputSource;

// https://immersive-web.github.io/hit-test/#xr-transient-input-hit-test-result-interface
class XRTransientInputHitTestResult : public RefCounted<XRTransientInputHitTestResult> {
    WTF_MAKE_ISO_ALLOCATED(XRTransientInputHitTestResult);
public:
    static Ref<XRTransientInputHitTestResult> create(WebXRFrame&, WebXRInputSource&, Vector<Ref<XRHitTestResult>>&&);

    const WebXRInputSource& inputSource() const;
    const Vector<Ref<XRHitTestResult>>& results() const;

protected:
    XRTransientInputHitTestResult(WebXRFrame&, WebXRInputSource&, Vector<Ref<XRHitTestResult>>&&);

private:
    Ref<WebXRFrame> m_frame;
    Ref<WebXRInputSource> m_inputSource;
    Vector<Ref<XRHitTestResult>> m_results;
};

} // namespace WebCore

#endif
