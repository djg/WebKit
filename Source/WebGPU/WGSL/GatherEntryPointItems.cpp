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
#include "GatherEntryPointItems.h"

#include "AST.h"
#include "ASTVisitor.h"
#include "Type.h"
#include "TypeContext.h"

namespace WGSL {

class Gatherer final : public AST::Visitor {
    using Base = AST::Visitor;

public:
    using Base::visit;

    Gatherer(Type::Context& context, const AST::Attribute* semantic = nullptr);

    Expected<EntryPointItems, Error> error();
    void reset();
    Vector<EntryPointItem> takeEntryPointItems();

    void visit(AST::ArrayType&) override;
    void visit(AST::NamedType&) override;
    void visit(AST::ParameterizedType&) override;
    void visit(AST::Parameter&) override;

private:
    SourceSpan m_currentLocation;
    const AST::Attribute* m_currentSemantic = nullptr;
    Vector<EntryPointItem> m_entryPointItems;
    Vector<StringView> m_path;
    Type::Context& m_context;
};

Gatherer::Gatherer(Type::Context& context, const AST::Attribute* semantic)
    : m_currentSemantic(semantic)
    , m_context(context)
{
}

Vector<EntryPointItem> Gatherer::takeEntryPointItems()
{
    return WTFMove(m_entryPointItems);
}

Expected<EntryPointItems, Error> Gatherer::error()
{
    return makeUnexpected(result().error());
}

void Gatherer::reset()
{
    m_currentLocation = SourceSpan {};
    m_currentSemantic = nullptr;
}

void Gatherer::visit(AST::ArrayType& /*arrayType*/)
{
    if (!m_currentSemantic)
        return setError({"Expected semantic for entrypoint argument."_str, m_currentLocation});
    m_entryPointItems.append({ m_path, /*arrayType*/ nullptr, m_currentSemantic });
}

void Gatherer::visit(AST::NamedType& namedType)
{
    if (!m_currentSemantic)
        return setError({"Expected semantic for entrypoint argument."_str, m_currentLocation});

    const Type::Node* resolvedType = m_context.lookup(namedType.name());
    ASSERT(resolvedType);
    m_entryPointItems.append({ m_path, resolvedType, m_currentSemantic });
}

void Gatherer::visit(AST::ParameterizedType& parameterizedType)
{
    if (!m_currentSemantic)
        return setError({"Expected semantic for entrypoint argument."_str, m_currentLocation});

    const Type::Node* resolvedType = m_context.lookup(parameterizedType.toString());
    ASSERT(resolvedType);
    m_entryPointItems.append({ m_path, resolvedType, m_currentSemantic });
}

void Gatherer::visit(AST::Parameter& parameter)
{
    ASSERT(!m_currentSemantic);
    if (parameter.maybeSemantic())
        m_currentSemantic = parameter.maybeSemantic();
    m_path.append(parameter.name());
    checkErrorAndVisit(parameter.type());
    m_path.takeLast();
}

EntryPointItem::EntryPointItem(Vector<StringView>& path, const Type::Node* type, const AST::Attribute* semantic)
    : path(path)
    , type(type)
    , semantic(semantic)
{
}

Expected<EntryPointItems, Error> gatherEntryPointItems(AST::FunctionDecl& functionDecl)
{
    Type::Context dummy{};

    ASSERT(functionDecl.maybeStage());
    Gatherer inputGatherer { dummy };
    for (auto& parameter : functionDecl.parameters()) {
        inputGatherer.reset();
        inputGatherer.visit(parameter);
        if (inputGatherer.hasError())
            return inputGatherer.error();
    }

    Gatherer outputGatherer { dummy, functionDecl.maybeReturnSemantic() };
    if (functionDecl.maybeReturnType() && functionDecl.stage() != AST::Stage::Compute)
        outputGatherer.checkErrorAndVisit(*functionDecl.maybeReturnType());
    if (outputGatherer.hasError())
        return outputGatherer.error();

    return {{ inputGatherer.takeEntryPointItems(), outputGatherer.takeEntryPointItems() }};
}

} // namespace WGSL
