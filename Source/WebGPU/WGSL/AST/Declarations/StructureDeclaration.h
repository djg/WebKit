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
#include "Attribute.h"
#include "CompilationMessage.h"
#include "Declaration.h"
#include "TypeName.h"
#include <wtf/FastMalloc.h>
#include <wtf/UniqueRef.h>
#include <wtf/text/StringView.h>

namespace WGSL::AST {

class StructureMember final : public ASTNode {
    WTF_MAKE_FAST_ALLOCATED;

public:
    using List = UniqueRefVector<StructureMember>;

    StructureMember(SourceSpan span, StringView name, TypeName::Ref&& type, Attribute::List&& attributes)
        : ASTNode(span)
        , m_name(name)
        , m_attributes(WTFMove(attributes))
        , m_type(WTFMove(type))
    {
    }

    const StringView& name() const { return m_name; }
    TypeName& type() { return m_type; }
    Attribute::List& attributes() { return m_attributes; }

private:
    StringView m_name;
    Attribute::List m_attributes;
    TypeName::Ref m_type;
};

class StructureDeclaration final : public Declaration {
    WTF_MAKE_FAST_ALLOCATED;

public:
    using Ref = UniqueRef<StructureDeclaration>;
    using List = UniqueRefVector<StructureDeclaration>;

    StructureDeclaration(SourceSpan sourceSpan, StringView name, StructureMember::List&& members)
        : Declaration(sourceSpan)
        , m_name(name)
        , m_attributes()
        , m_members(WTFMove(members))
    {
    }

    Kind kind() const override { return Kind::Structure; }
    const StringView& name() const { return m_name; }
    Attribute::List& attributes() { return m_attributes; }
    StructureMember::List& members() { return m_members; }

private:
    StringView m_name;
    Attribute::List m_attributes;
    StructureMember::List m_members;
};

} // namespace WGSL::AST

SPECIALIZE_TYPE_TRAITS_WGSL_DECL(StructureDeclaration, isStructure())
