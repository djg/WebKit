/*
 * Copyright (c) 2021 Apple Inc. All rights reserved.
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

#pragma once

#include "ASTNode.h"
#include "Declaration.h"
#include "FunctionDeclaration.h"
#include "GlobalDirective.h"
#include "StructureDeclaration.h"
#include "VariableDeclaration.h"
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WGSL::AST {

class ShaderModule {
    WTF_MAKE_FAST_ALLOCATED;

public:
    ShaderModule() = default;
    ShaderModule(ShaderModule&&) = default;

    GlobalDirective::List& directives() { return m_directives; }
    StructureDeclaration::List& structures() { return m_structures; }
    VariableDeclaration::List& variables() { return m_variables; }
    FunctionDeclaration::List& functions() { return m_functions; }

    const GlobalDirective::List& directives() const { return m_directives; }
    const StructureDeclaration::List& structs() const { return m_structures; }
    const VariableDeclaration::List& variables() const { return m_variables; }
    const FunctionDeclaration::List& functions() const { return m_functions; }

private:
    GlobalDirective::List m_directives;
    StructureDeclaration::List m_structures;
    VariableDeclaration::List m_variables;
    FunctionDeclaration::List m_functions;
};

} // namespace WGSL::AST
