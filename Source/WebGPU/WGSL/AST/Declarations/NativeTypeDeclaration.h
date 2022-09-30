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

#include "Attribute.h"
#include "Declaration.h"
#include "TypeName.h"

namespace WGSL::AST {

class NativeTypeDeclaration : public Declaration {
    WTF_MAKE_FAST_ALLOCATED;

public:
    using Ref = UniqueRef<NativeTypeDeclaration>;
    using List = UniqueRefVector<NativeTypeDeclaration>;

    NativeTypeDeclaration(SourceSpan span, TypeName::Ref&& typeName)
        : Declaration(span)
        , m_typeName(WTFMove(typeName))
    {
    }

    virtual ~NativeTypeDeclaration() {}

    virtual Kind kind() const { return Declaration::Kind::NativeType; }

    Attribute::List& attributes() { return m_attributes; }
    TypeName& type() { return m_typeName.get(); }

private:
    Attribute::List m_attributes;
    TypeName::Ref m_typeName;
};

} // namespace WGSL::AST

SPECIALIZE_TYPE_TRAITS_WGSL_DECL(NativeTypeDeclaration, isNativeType())
