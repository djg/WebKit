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
#include <wtf/UniqueRef.h>
#include <wtf/FastMalloc.h>

namespace WGSL::AST {

class Literal : public ASTNode {
    WTF_MAKE_FAST_ALLOCATED;
public:
    enum class Kind {
        Boolean,
        Float,
        Integer,
    };

    using Ref = UniqueRef<Literal>;

    Literal(SourceSpan span)
        : ASTNode(span)
    {
    }

    virtual ~Literal() = default;

    virtual Kind kind() const = 0;
    bool isBoolean() const { return kind() == Kind::Boolean; }
    bool isInteger() const { return kind() == Kind::Integer; }
    bool isFloat() const { return kind() == Kind::Float; }

protected:
};

class BoolLiteral final : public Literal {
    WTF_MAKE_FAST_ALLOCATED;
public:
    BoolLiteral(SourceSpan span, bool value)
        : Literal(span)
        , m_value(value)
    {
    }

    Kind kind() const override { return Kind::Boolean; }
    bool value() const { return m_value; }

private:
    bool m_value;
};

class FloatLiteral : public Literal {
    WTF_MAKE_FAST_ALLOCATED;
public:
    enum class Suffix : uint8_t {
        None,
        Float32,
    };

    FloatLiteral(SourceSpan span, double value, Suffix suffix = Suffix::None)
        : Literal(span)
        , m_value(value)
        , m_suffix(suffix)
    {
    }

    Kind kind() const override { return Kind::Float; }
    Suffix suffix() const { return m_suffix; }
    double value() const { return m_value; }

private:
    double m_value;
    Suffix m_suffix;
};

class IntegerLiteral final : public Literal {
    WTF_MAKE_FAST_ALLOCATED;
public:

    enum class Suffix : uint8_t {
        None,
        Int32,
        Uint32,
    };

    IntegerLiteral(SourceSpan span, double value, Suffix suffix = Suffix::None)
        : Literal(span)
        , m_value(value)
        , m_suffix(suffix)
    {
    }

    Kind kind() const override { return Kind::Integer; }
    Suffix suffix() const { return m_suffix; }
    double value() const { return m_value; }

private:
    double m_value;
    Suffix m_suffix;
};

} // namespace WGSL::AST

#define SPECIALIZE_TYPE_TRAITS_WGSL_LITERAL(ToValueTypeName, predicate)                 \
    SPECIALIZE_TYPE_TRAITS_BEGIN(WGSL::AST::ToValueTypeName)                            \
    static bool isType(const WGSL::AST::Literal& literal) { return literal.predicate; } \
    SPECIALIZE_TYPE_TRAITS_END()

SPECIALIZE_TYPE_TRAITS_WGSL_LITERAL(BoolLiteral, isBoolean())
SPECIALIZE_TYPE_TRAITS_WGSL_LITERAL(FloatLiteral, isFloat())
SPECIALIZE_TYPE_TRAITS_WGSL_LITERAL(IntegerLiteral, isInteger())
