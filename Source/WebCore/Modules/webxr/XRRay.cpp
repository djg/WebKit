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
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "XRRay.h"

#if ENABLE(WEBXR_HIT_TEST)

#include "DOMPointReadOnly.h"

namespace WebCore {

static Quaternion fromAxisAngle(const DOMPointInit& aa)
{
    double s = std::sin(aa.w / 2);
    double c = std::cos(aa.w / 2);
    return { aa.x * s, aa.y * s, aa.z * s, c };
}

static void normalizeDirection(XRRay::DirectionInit& d)
{
    const double length = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    d.x /= length;
    d.y /= length;
    d.z /= length;
}

static inline void mapDOMPointInit(const TransformationMatrix& transform, DOMPointInit& point)
{
    transform.map4ComponentPoint(point.x, point.y, point.z, point.w);
}

WTF_MAKE_ISO_ALLOCATED_IMPL(XRRay);

XRRay::XRRay(const DOMPointInit& origin, const DOMPointInit& direction)
    : m_origin(DOMPointReadOnly::create(origin))
    , m_direction(DOMPointReadOnly::create(direction))
{ }

Ref<XRRay> XRRay::create()
{
    DOMPointInit origin { };
    DirectionInit direction { };
    return adoptRef(*new XRRay(origin, direction));
}

ExceptionOr<Ref<XRRay>> XRRay::create(const DOMPointInit& originInit, const DirectionInit& directionInit)
{
    // 1. Let ray be a new XRRay.
    // 2. Initialize ray’s origin to { x: 0.0, y: 0.0, z: 0.0, w: 1.0 }.
    // 3. Initialize ray’s direction to { x: 0.0, y: 0.0, z: -1.0, w: 0.0 }.
    // 4. If all of direction’s x, y, and z are zero, throw a TypeError and abort these steps.
    if (directionInit.x == 0 && directionInit.y == 0 && directionInit.z == 0)
        return Exception { ExceptionCode::TypeError };

    // 5. If direction’s w is not 0.0, throw a TypeError and abort these steps.
    if (directionInit.w != 0)
        return Exception { ExceptionCode::TypeError };

    // 6. If origin’s w is not 1.0, throw a TypeError and abort these steps.
    if (originInit.w != 1)
        return Exception { ExceptionCode::TypeError };

    // 7. Initialize ray’s origin’s x value to origin’s x, y value to origin’s y, and z value to origin’s z.
    auto origin = originInit;

    // 8. Initialize ray’s direction’s x value to direction’s x, y value to direction’s y, and z value to direction’s z.
    auto direction = directionInit;

    // 9. Normalize the x, y, and z components of ray’s direction.
    normalizeDirection(direction);

    // 10. Initialize ray’s matrix to null.
    // 11. Return ray.
    return adoptRef(*new XRRay(origin, direction));
}

ExceptionOr<Ref<XRRay>> XRRay::create(const WebXRRigidTransform& rigidTransform)
{
    // 1. Let ray be a new XRRay.
    // 2. Initialize ray’s origin to { x: 0.0, y: 0.0, z: 0.0, w: 1.0 }.
    DOMPointInit origin { };

    // 3. Initialize ray’s direction to { x: 0.0, y: 0.0, z: -1.0, w: 0.0 }.
    DirectionInit direction { };

    auto& transform = rigidTransform.rawTransform();
    // 4. Transform ray’s origin by premultiplying the transform’s matrix and set ray to the result.
    transform.map4ComponentPoint(origin.x, origin.y, origin.z, origin.w);

    // 5. Transform ray’s direction by premultiplying the transform’s matrix and set ray to the result.
    transform.map4ComponentPoint(direction.x, direction.y, direction.z, direction.w);

    // 6. Normalize the x, y, and z components of ray’s direction.
    normalizeDirection(direction);

    // 7. Initialize ray’s matrix to null.
    // 8. Return ray.
    return adoptRef(*new XRRay(origin, direction));
}

Ref<XRRay> XRRay::from(const TransformationMatrix& transform, const XRRay& ray)
{
    auto origin = (DOMPointInit) ray.origin();
    auto direction = (DOMPointInit) ray.direction();
    mapDOMPointInit(transform, origin);
    mapDOMPointInit(transform, direction);
    return adoptRef(*new XRRay(origin, direction));
}

const DOMPointReadOnly& XRRay::origin() const
{
    return m_origin;
}

const DOMPointReadOnly& XRRay::direction() const
{
    return m_direction;
}

const Float32Array& XRRay::matrix()
{
    // 1. If ray’s matrix is not null, perform the following steps:
    //     * If the operation IsDetachedBuffer on matrix is false, return ray’s matrix.
    if (m_matrix && !m_matrix->isDetached())
        return *m_matrix;

    // 2. Let z be the vector [0, 0, -1].
    // 3. Let axis be the vector cross product of z and ray’s direction, z × direction.
    // **NOTE**: axis = [direction.y, -direction.x, 0]
    // z × direction => [ 0.direction.z - -1.direction.y, -1.direction.x - 0.direction.z, 0.direction.y - 0.direction.x ]
    //               => [ direction.y, -direction.x, 0 ]

    // 4. Let cos_angle be the scalar dot product of z and ray’s direction, z · direction.
    // **NOTE**: cos_angle = -direction.z
    // z · direction => (0*direction.x + 0*direction.y + -1*direction.z)
    //               => -direction.z

    // 5. Set rotation based on the following:
    // 5.1 If cos_angle is greater than -1 and less than 1:
    // **NOTE**: since cos_angle == -direction.z, -1 < cos_angle && cos_angle < 1 implies -1 < direction.z && direction.z < 1
    //     * Set rotation to the rotation matrix representing a right handed planar rotation around axis by arccos(cos_angle).
    // 5.2 Else, if cos_angle is -1
    // **NOTE**: since cos_angle == -direction.z, cos_angle == -1 implies direction.z == 1
    //     * Set rotation to the rotation matrix representing a right handed planar rotation around vector [1, 0, 0] by arccos(cos_angle).
    // 5.3 Else
    //     * Set rotation to an identity matrix.

    double dx = m_direction->x();
    double dy = m_direction->y();
    double dz = m_direction->z();
    DOMPointInit axis_angle = { 0, 0, 0, 0 };
    if (-1 < dz&& dz< 1) {
        axis_angle.x = dy;
        axis_angle.y = -dx;
        axis_angle.z = std::acos(-dz);
    } else if (dz == 1) {
        axis_angle.x = 1;
        axis_angle.z = std::acos(-dz);
    }

    auto transform = TransformationMatrix::fromQuaternion(fromAxisAngle(axis_angle));

    // 6. Let translation be the translation matrix with components corresponding to ray’s origin.
    // 7. Let matrix be the result of premultiplying rotation from the left onto translation (i.e. translation * rotation) in column-vector notation.
    // **NOTE**: translation * rotation applies rotation then translation which is the equivalent of just setting the translation elements.
    transform.setM41(m_origin->x());
    transform.setM42(m_origin->y());
    transform.setM43(m_origin->z());

    // 8. Set ray’s matrix to matrix.
    auto matrixData = transform.toColumnMajorFloatArray();
    m_matrix = Float32Array::create(matrixData.data(), matrixData.size());

    // 9. Return matrix.
    return *m_matrix;
}

} // namespace WebCore

#endif
