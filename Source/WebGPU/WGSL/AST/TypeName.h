/*
 * Copyright (C) 2022 Apple Inc. All rights reserved.
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

#include "ASTNode.h"
#include "Expression.h"
#include <wtf/TypeCasts.h>

namespace WGSL::AST {

class TypeName : public ASTNode {
    WTF_MAKE_FAST_ALLOCATED;

public:
    enum class Kind : uint8_t {
        Array,
        Named,
        Parameterized,
    };

    TypeName(SourceSpan span)
        : ASTNode(span)
    {
    }

    virtual ~TypeName() {}

    virtual Kind kind() const = 0;
    bool isArray() const { return kind() == Kind::Array; }
    bool isNamed() const { return kind() == Kind::Named; }
    bool isParameterized() const { return kind() == Kind::Parameterized; }
};

class ArrayTypeName final : public TypeName {
    WTF_MAKE_FAST_ALLOCATED;
public:
    ArrayTypeName(SourceSpan span, std::unique_ptr<TypeName>&& elementType, std::unique_ptr<Expression>&& elementCount)
        : TypeName(span)
        , m_elementType(WTFMove(elementType))
        , m_elementCount(WTFMove(elementCount))
    {
    }

    Kind kind() const override { return Kind::Array; }
    TypeName* maybeElementType() const { return m_elementType.get(); }
    Expression* maybeElementCount() const { return m_elementCount.get(); }

private:
    std::unique_ptr<TypeName> m_elementType;
    std::unique_ptr<Expression> m_elementCount;
};

class NamedTypeName final : public TypeName {
    WTF_MAKE_FAST_ALLOCATED;
public:
    NamedTypeName(SourceSpan span, StringView&& name)
        : TypeName(span)
        , m_name(WTFMove(name))
    {
    }

    Kind kind() const override { return Kind::Named; }
    const StringView& name() const { return m_name; }

private:
    StringView m_name;
};

class ParameterizedTypeName : public TypeName {
    WTF_MAKE_FAST_ALLOCATED;
public:
    enum class Base {
        Vec2,
        Vec3,
        Vec4,
        Mat2x2,
        Mat2x3,
        Mat2x4,
        Mat3x2,
        Mat3x3,
        Mat3x4,
        Mat4x2,
        Mat4x3,
        Mat4x4
    };

    ParameterizedTypeName(SourceSpan span, Base base, UniqueRef<TypeName>&& elementType)
        : TypeName(span)
        , m_base(base)
        , m_elementType(WTFMove(elementType))
    {
    }

    static std::optional<Base> stringViewToKind(StringView& view)
    {
        if (view == "vec2"_s)
            return Base::Vec2;
        if (view == "vec3"_s)
            return Base::Vec3;
        if (view == "vec4"_s)
            return Base::Vec4;
        if (view == "mat2x2"_s)
            return Base::Mat2x2;
        if (view == "mat2x3"_s)
            return Base::Mat2x3;
        if (view == "mat2x4"_s)
            return Base::Mat2x4;
        if (view == "mat3x2"_s)
            return Base::Mat3x2;
        if (view == "mat3x3"_s)
            return Base::Mat3x3;
        if (view == "mat3x4"_s)
            return Base::Mat3x4;
        if (view == "mat4x2"_s)
            return Base::Mat4x2;
        if (view == "mat4x3"_s)
            return Base::Mat4x3;
        if (view == "mat4x4"_s)
            return Base::Mat4x4;
        return std::nullopt;
    }

    Kind kind() const override { return Kind::Parameterized; }
    Base base() const { return m_base; }
    TypeName& elementType() { return m_elementType; }

private:
    Base m_base;
    UniqueRef<TypeName> m_elementType;
};

} // namespace WGSL::AST

#define SPECIALIZE_TYPE_TRAITS_WGSL_TYPE(ToValueTypeName, predicate) \
SPECIALIZE_TYPE_TRAITS_BEGIN(WGSL::AST::ToValueTypeName) \
    static bool isType(const WGSL::AST::TypeName& type) { return type.predicate; } \
SPECIALIZE_TYPE_TRAITS_END()

SPECIALIZE_TYPE_TRAITS_WGSL_TYPE(ArrayTypeName, isArray())
SPECIALIZE_TYPE_TRAITS_WGSL_TYPE(NamedTypeName, isNamed())
SPECIALIZE_TYPE_TRAITS_WGSL_TYPE(ParameterizedTypeName, isParameterized())
