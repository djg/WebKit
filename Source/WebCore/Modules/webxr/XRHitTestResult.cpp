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

#include "config.h"
#include "XRHitTestResult.h"

#if ENABLE(WEBXR_HIT_TEST)

#include "WebXRFrame.h"
#include "WebXRPose.h"
#include <wtf/IsoMallocInlines.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(XRHitTestResult);

XRHitTestResult::XRHitTestResult(WebXRFrame& frame, std::optional<TransformationMatrix>&& nativeOrigin)
    : m_frame(frame)
    , m_nativeOrigin(WTFMove(nativeOrigin))
{ }

Ref<XRHitTestResult> XRHitTestResult::create(WebXRFrame& frame, std::optional<TransformationMatrix>&& nativeOrigin)
{
    return adoptRef(*new XRHitTestResult(frame, WTFMove(nativeOrigin)));
}

ExceptionOr<RefPtr<WebXRPose>> XRHitTestResult::getPose(const WebXRSpace&)
{
    // Let frame be the hitTestResult’s frame.
    // If frame’s active boolean is false, throw an InvalidStateError and abort these steps.
    if (!m_frame->isActive())
        return Exception { ExceptionCode::InvalidStateError };

    // Let pose be a new XRPose.
    // Let space be a new XRSpace, with native origin set to native origin, origin offset set to identity transform, and session set to frame's session.
    // Populate the pose of space in baseSpace at the time represented by frame into pose.
    // Return pose.
    ASSERT_NOT_IMPLEMENTED_YET();
    return nullptr;
}

} // namespace WebCore

#endif

