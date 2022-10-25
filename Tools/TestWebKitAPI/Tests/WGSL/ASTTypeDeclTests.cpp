/*
 * Copyright (c) 2022 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ASTTypeDecl.h"

#include <wtf/text/StringView.h>
#include <wtf/text/WTFString.h>

namespace TestWGSLAPI {

WGSL::AST::ParameterizedType::Ref makeParameterizedType(WGSL::AST::ParameterizedType::Base base)
{
    auto elementType = makeUniqueRef<WGSL::AST::NamedType>(WGSL::SourceSpan{}, StringView("f32"_s));
    return makeUniqueRef<WGSL::AST::ParameterizedType>(WGSL::SourceSpan{}, base, WTFMove(elementType));
}

struct ToStringTestCase {
    WGSL::AST::TypeDecl::Ref input;
    ASCIILiteral expectedValue;
};

TEST(WGSLASTTypeDecl, ParameterizedTypeToString)
{
    using namespace WGSL;

    static const ToStringTestCase testCases[] = {
        { makeParameterizedType(AST::ParameterizedType::Base::Vec2), "vec2<f32>"_s },
        { makeParameterizedType(AST::ParameterizedType::Base::Vec3), "vec3<f32>"_s },
        { makeParameterizedType(AST::ParameterizedType::Base::Vec4), "vec4<f32>"_s },
        { makeParameterizedType(AST::ParameterizedType::Base::Mat2x2), "mat2x2<f32>"_s },
        { makeParameterizedType(AST::ParameterizedType::Base::Mat2x3), "mat2x3<f32>"_s },
        { makeParameterizedType(AST::ParameterizedType::Base::Mat2x4), "mat2x4<f32>"_s },
        { makeParameterizedType(AST::ParameterizedType::Base::Mat3x2), "mat3x2<f32>"_s },
        { makeParameterizedType(AST::ParameterizedType::Base::Mat3x3), "mat3x3<f32>"_s },
        { makeParameterizedType(AST::ParameterizedType::Base::Mat3x4), "mat3x4<f32>"_s },
        { makeParameterizedType(AST::ParameterizedType::Base::Mat4x2), "mat4x2<f32>"_s },
        { makeParameterizedType(AST::ParameterizedType::Base::Mat4x3), "mat4x3<f32>"_s },
        { makeParameterizedType(AST::ParameterizedType::Base::Mat4x4), "mat4x4<f32>"_s },
    };

    for (const auto& testCase : testCases) {
        EXPECT_EQ(testCase.input->toString(), testCase.expectedValue);
    }
}

} // namespace TestWGSLAPI
