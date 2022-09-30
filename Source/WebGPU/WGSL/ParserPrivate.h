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

#include "AST.h"
#include "CompilationMessage.h"
#include "Lexer.h"
#include "Parser.h"

namespace WGSL {

template<typename T>
using Result = Expected<T, Error>;

template<typename Lexer>
class Parser {
public:
    Parser(Lexer& lexer, ParsingMode mode)
        : m_lexer(lexer)
        , m_current(lexer.lex())
        , m_mode(mode)
    {
    }

    Result<AST::ShaderModule> parseShader();

    // UniqueRef whenever it can return multiple types.
    Result<AST::Attribute::List> parseAttributes();
    Result<AST::Attribute::Ref> parseAttribute();
    Result<AST::StructureDeclaration::Ref> parseStructureDeclaration();
    Result<AST::StructureMember::Ref> parseStructureMember();
    Result<AST::TypeName::Ref> parseTypeName();
    Result<AST::TypeName::Ref> parseTypeNameAfterIdentifier(StringView&&);
    Result<AST::TypeName::Ref> parseArrayTypeName();
    Result<AST::NativeTypeDeclaration::Ref> parseNativeTypeDeclaration();
    Result<AST::TypeDeclaration::Ref> parseTypeDeclaration();
    Result<AST::VariableDeclaration::Ref> parseVariableDeclaration();
    Result<AST::VariableQualifier> parseVariableQualifier();
    Result<AST::StorageClass> parseStorageClass();
    Result<AST::AccessMode> parseAccessMode();
    Result<AST::FunctionDeclaration::Ref> parseFunctionDeclaration();
    Result<AST::Parameter> parseParameter();
    Result<AST::Statement::Ref> parseStatement();
    Result<AST::CompoundStatement::Ref> parseCompoundStatement();
    Result<AST::ReturnStatement::Ref> parseReturnStatement();
    Result<AST::Expression::Ref> parseShortCircuitOrExpression();
    Result<AST::Expression::Ref> parseRelationalExpression();
    Result<AST::Expression::Ref> parseShiftExpression();
    Result<AST::Expression::Ref> parseAdditiveExpression();
    Result<AST::Expression::Ref> parseMultiplicativeExpression();
    Result<AST::Expression::Ref> parseUnaryExpression();
    Result<AST::Expression::Ref> parseSingularExpression();
    Result<AST::Expression::Ref> parsePostfixExpression(AST::Expression::Ref&& base, SourcePosition startPosition);
    Result<AST::Expression::Ref> parsePrimaryExpression();
    Result<AST::Expression::Ref> parseExpression();
    Result<AST::Expression::Ref> parseLHSExpression();
    Result<AST::Expression::Ref> parseCoreLHSExpression();
    Result<AST::Expression::List> parseArgumentExpressionList();

private:
    Expected<Token, TokenType> consumeType(TokenType);
    void consume();

    Token& current() { return m_current; }
    SourcePosition startPosition() const { return m_startOfElementPosition; }
    SourceSpan currentSpan() const { return SourceSpan(startPosition(), m_lexer.currentPosition()); }

    template<typename T, typename... Args>
    UniqueRef<T> make(Args&&...);

    SourcePosition m_startOfElementPosition = {};

    Lexer& m_lexer;
    Token m_current;
    ParsingMode m_mode;
};

} // namespace WGSL
