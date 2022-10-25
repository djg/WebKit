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

#include "ASTTypeDecl.h"

#include <wtf/Forward.h>
#include <wtf/text/StringBuilder.h>

namespace WTF {
template<> class StringTypeAdapter<WGSL::AST::ParameterizedType::Base> {
    static constexpr ASCIILiteral names[] = {
        "vec2"_s,
        "vec3"_s,
        "vec4"_s,
        "mat2x2"_s,
        "mat2x3"_s,
        "mat2x4"_s,
        "mat3x2"_s,
        "mat3x3"_s,
        "mat3x4"_s,
        "mat4x2"_s,
        "mat4x3"_s,
        "mat4x4"_s,
    };

public:
    StringTypeAdapter(WGSL::AST::ParameterizedType::Base base)
        : m_base(base)
    {
    }

    bool is8Bit() const { return true; }

    unsigned length() const
    {
        return StringTypeAdapter<ASCIILiteral>(names[static_cast<unsigned>(m_base)]).length();
    }

    template<typename CharacterType> void writeTo(CharacterType* destination) const
    {
        StringTypeAdapter<ASCIILiteral>(names[static_cast<unsigned>(m_base)]).writeTo(destination);
    }

private:
    WGSL::AST::ParameterizedType::Base m_base;
};

} // namespace WTF

namespace WGSL::AST {

String ArrayType::toString() const
{
    StringBuilder sb;
    sb.append("array<");
    if (maybeElementType()) {
        sb.append(maybeElementType()->toString());
        auto count = elementCount();
        if (count)
            sb.append(",", count);
    }
    sb.append(">");
    return sb.toString();
}

String NamedType::toString() const
{
    return m_name.toString();
}

String ParameterizedType::toString() const
{
    StringBuilder sb;
    sb.append(base(), "<", elementType().toString(), ">");
    return sb.toString();
}

} // namespace WGSL::AST
