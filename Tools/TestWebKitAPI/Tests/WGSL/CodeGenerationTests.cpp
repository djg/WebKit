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

#include "AST.h"
#include "Lexer.h"
#include "Parser.h"
#include "TestWGSLAPI.h"
#include "Type.h"
#include "WGSL.h"
#include "GatherEntryPointItems.h"

static constexpr ASCIILiteral gTrivialGraphicsShader = R"(@vertex
fn main(
    @builtin(vertex_index) VertexIndex : u32
) -> @builtin(position) vec4<f32> {
    var pos = array<vec2<f32>, 3>(
        vec2<f32>( 0.0, 0.5),
        vec2<f32>(-0.5,-0.5),
        vec2<f32>( 0.5,-0.5)
    );
    return vec4<f32>(pos[VertexIndex], 0.0, 1.0);
}

@fragment
fn main() -> @location(0) vec4<f32> {
    return vec4<f32>(1.0, 0.0, 0.0, 1.0);
})"_s;

TEST(WGSLCodeGenerationTests, TrivialGraphicsShader)
{
    auto shader = WGSL::parseLChar(gTrivialGraphicsShader);

    EXPECT_SHADER(shader);
    EXPECT_EQ(shader->functions().size(), 2u);

    {
        auto entryPointItems = WGSL::gatherEntryPointItems(shader->functions()[0]);
        if (!entryPointItems.has_value())
            return ::TestWGSLAPI::logCompilationError(shader.error());
        EXPECT_EQ(entryPointItems->inputs.size(), 1U);
        auto& inputItem = entryPointItems->inputs[0];
        EXPECT_EQ(inputItem.path.size(), 1U);
        EXPECT_EQ(inputItem.path[0], "VertexIndex"_s);
        EXPECT_TRUE(inputItem.semantic);
        EXPECT_TRUE(is<WGSL::AST::BuiltinAttribute>(inputItem.semantic));
        auto& inputSemantic = downcast<WGSL::AST::BuiltinAttribute>(*inputItem.semantic);
        EXPECT_EQ(inputSemantic.name(), "vertex_index"_s);
        EXPECT_TRUE(inputItem.type);
        EXPECT_TRUE(is<WGSL::Type::Unsigned32>(inputItem.type));

        EXPECT_EQ(entryPointItems->outputs.size(), 1U);
    }

    {
        auto entryPointItems = WGSL::gatherEntryPointItems(shader->functions()[1]);
        if (!entryPointItems.has_value())
            return ::TestWGSLAPI::logCompilationError(shader.error());
        EXPECT_TRUE(entryPointItems->inputs.isEmpty());
        EXPECT_EQ(entryPointItems->outputs.size(), 1U);
    }
}
