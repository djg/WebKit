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

#include "config.h"
#include "ASTFunctionDecl.h"

#include "ASTBuiltinAttribute.h"
#include "ASTLocationAttribute.h"
#include "ASTStageAttribute.h"

namespace WGSL::AST {

static const Attribute* firstSemantic(const Attribute::List& attributes)
{
    auto index = attributes.findIf([](const auto& attr) {
        return is<BuiltinAttribute>(attr) || is<LocationAttribute>(attr);
    });
    if (index == notFound)
        return nullptr;
    return &attributes[index];
}

const Attribute* Parameter::maybeSemantic() const
{
    return firstSemantic(m_attributes);
}

std::optional<Stage> FunctionDecl::maybeStage() const
{
    auto index = m_attributes.findIf([](const Attribute& attr) { return is<StageAttribute>(attr); });
    if (index == notFound)
        return {};

    return { downcast<WGSL::AST::StageAttribute>(m_attributes[index]).stage() };
}

Stage FunctionDecl::stage() const
{
    auto stage = maybeStage();
    ASSERT(stage);
    return stage.value();
}

const Attribute* FunctionDecl::maybeReturnSemantic() const
{
    return firstSemantic(m_returnAttributes);
}

} // namespace WGSL::AST
