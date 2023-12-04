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
#include "XRLightEstimate.h"

#if ENABLE(WEBXR_LIGHT_ESTIMATION)

#include "DOMPointReadOnly.h"
#include <JavaScriptCore/TypedArrayInlines.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(XRLightEstimate);

XRLightEstimate::XRLightEstimate(Ref<DOMPointReadOnly>&& primaryLightDirection, Ref<DOMPointReadOnly>&& primaryLightIntensity)
    : m_primaryLightDirection(WTFMove(primaryLightDirection))
    , m_primaryLightIntensity(WTFMove(primaryLightIntensity))
{ }

XRLightEstimate:: ~XRLightEstimate() = default;

Ref<XRLightEstimate> XRLightEstimate::create(const DOMPointInit& primaryLightDirection, const DOMPointInit& primaryLightIntensity)
{
    return adoptRef(*new XRLightEstimate(DOMPointReadOnly::create(primaryLightDirection), DOMPointReadOnly::create(primaryLightIntensity)));
}

const Float32Array& XRLightEstimate::sphericalHarmonicsCoefficients()
{
    if (m_sphericalHarmonicsCoefficients && !m_sphericalHarmonicsCoefficients->isDetached())
        return *m_sphericalHarmonicsCoefficients;

    // Lazily create coefficients Float32Array
    m_sphericalHarmonicsCoefficients = Float32Array::create(nullptr, 27);
    return *m_sphericalHarmonicsCoefficients;
}

const DOMPointReadOnly& XRLightEstimate::primaryLightDirection() const
{
    return m_primaryLightDirection;
}

const DOMPointReadOnly& XRLightEstimate::primaryLightIntensity() const
{
    return m_primaryLightIntensity;
}

} // namespace WebCore

#endif
