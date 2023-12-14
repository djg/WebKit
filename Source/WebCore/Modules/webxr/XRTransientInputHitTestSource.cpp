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
#include "XRTransientInputHitTestSource.h"

#if ENABLE(WEBXR_HIT_TEST)

#include "DOMPointReadOnly.h"
#include "WebXRSession.h"
#include "XRRay.h"

#include <wtf/IsoMallocInlines.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(XRTransientInputHitTestSource);

XRTransientInputHitTestSource::XRTransientInputHitTestSource(WebXRSession& session, String&& profile, Vector<XRHitTestTrackableType>&& entityTypes, XRRay& offsetRay)
    : m_session(session)
    , m_profile(profile)
    , m_entityTypes(WTFMove(entityTypes))
    , m_offsetRay(offsetRay)
{ }

Ref<XRTransientInputHitTestSource> XRTransientInputHitTestSource::create(WebXRSession& session, String&& profile, Vector<XRHitTestTrackableType>&& entityTypes, XRRay& offsetRay)
{
    // 1. Let hitTestSource be a new XRTransientInputHitTestSource.
    // 2. Initialize hitTestSource’s session to session.
    // 3. Initialize hitTestSource’s profile to profile.
    // 4. Initialize hitTestSource’s entity types to entityTypes.
    // 5. Initialize hitTestSource’s offset ray to offsetRay.
    // 6. Return hitTestSource.
    return adoptRef(*new XRTransientInputHitTestSource(session, WTFMove(profile), WTFMove(entityTypes), offsetRay));
}

ExceptionOr<void> XRTransientInputHitTestSource::cancel()
{
    return m_session->cancelHitTestSourceForTransientInput(*this);
}

} // namespace WebCore

#endif
