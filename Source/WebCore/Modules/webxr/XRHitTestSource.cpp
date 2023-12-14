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
#include "XRHitTestSource.h"

#if ENABLE(WEBXR_HIT_TEST)

#include "WebXRSession.h"
#include "WebXRSpace.h"
#include "XRRay.h"

#include <wtf/IsoMallocInlines.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(XRHitTestSource);

XRHitTestSource::XRHitTestSource(WebXRSession& session, std::optional<TransformationMatrix>&& nativeOrigin, Vector<XRHitTestTrackableType>&& entityTypes, XRRay& offsetRay)
    : m_session(session)
    , m_nativeOrigin(WTFMove(nativeOrigin))
    , m_entityTypes(WTFMove(entityTypes))
    , m_offsetRay(offsetRay)
{ }

Ref<XRHitTestSource> XRHitTestSource::create(WebXRSession& session, const WebXRSpace& space, Vector<XRHitTestTrackableType>&& entityTypes, XRRay& offsetRay)
{
    // 1. Let hitTestSource be a new XRHitTestSource.
    // 2. Initialize hitTestSource’s session to session.
    // 3. Initialize hitTestSource’s native origin to space’s native origin.
    auto nativeOrigin = space.nativeOrigin();

    // 4. Initialize hitTestSource’s entity types to entityTypes.
    // 5. Compute transformedOffsetRay from offsetRay and space such that transformedOffsetRay, when interpreted in space’s native origin coordinate system, represents the same ray as offsetRay does when interpreted in space’s effective origin coordinate system.

    Ref transformedOffsetRay = offsetRay;
    auto maybeNativeOrigin = space.nativeOrigin();
    auto maybeEffectiveOrigin = space.effectiveOrigin();
    RELEASE_ASSERT(maybeNativeOrigin);
    RELEASE_ASSERT(maybeEffectiveOrigin);
    if (maybeNativeOrigin && maybeEffectiveOrigin) {
        RELEASE_ASSERT((*maybeNativeOrigin).isInvertible());
        auto transform = *(*maybeNativeOrigin).inverse() * (*maybeEffectiveOrigin);
        transformedOffsetRay = XRRay::from(transform, offsetRay);
    }

    // 6. Initialize hitTestSource’s offset ray to transformedOffsetRay.
    // 7. Return hitTestSource.
    return adoptRef(*new XRHitTestSource(session, WTFMove(nativeOrigin), WTFMove(entityTypes), transformedOffsetRay));
}

ExceptionOr<void> XRHitTestSource::cancel()
{
    return m_session->cancelHitTestSource(*this);
}

} // namespace WebCore

#endif
