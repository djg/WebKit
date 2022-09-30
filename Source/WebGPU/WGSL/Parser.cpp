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

#include "Parser.h"
#include "ParserPrivate.h"

#include "Lexer.h"
#include <wtf/text/StringBuilder.h>

namespace WGSL {

#define START_PARSE() \
    auto _startOfElementPosition = m_lexer.currentPosition();

#define CURRENT_SOURCE_SPAN() \
    SourceSpan(_startOfElementPosition, m_lexer.currentPosition())

#define RETURN_NODE(type, ...)                                       \
    do {                                                             \
        AST::type astNodeResult(CURRENT_SOURCE_SPAN(), __VA_ARGS__); \
        return { WTFMove(astNodeResult) };                           \
    } while (false)

// Passing 0 arguments beyond the type to RETURN_NODE is invalid because of a stupid limitation of the C preprocessor
#define RETURN_NODE_NO_ARGS(type)                       \
    do {                                                \
        AST::type astNodeResult(CURRENT_SOURCE_SPAN()); \
        return { WTFMove(astNodeResult) };              \
    } while (false)

#define RETURN_NODE_REF(type, ...) \
    return { makeUniqueRef<AST::type>(CURRENT_SOURCE_SPAN(), __VA_ARGS__) };

// Passing 0 arguments beyond the type to RETURN_NODE_REF is invalid because of a stupid limitation of the C preprocessor
#define RETURN_NODE_REF_NO_ARGS(type) \
    return { makeUniqueRef<AST::type>(CURRENT_SOURCE_SPAN()) };

#define FAIL(string) \
    return makeUnexpected(Error(string, CURRENT_SOURCE_SPAN()));

// Warning: cannot use the do..while trick because it defines a new identifier named `name`.
// So do not use after an if/for/while without braces.
#define PARSE(name, element, ...)                      \
    auto name##Expected = parse##element(__VA_ARGS__); \
    if (!name##Expected)                               \
        return makeUnexpected(name##Expected.error()); \
    auto& name = *name##Expected;

// Warning: cannot use the do..while trick because it defines a new identifier named `name`.
// So do not use after an if/for/while without braces.
#define CONSUME_TYPE_NAMED(name, type)                    \
    auto name##Expected = consumeType(TokenType::type);   \
    if (!name##Expected) {                                \
        StringBuilder builder;                            \
        builder.append("Expected a ");                    \
        builder.append(toString(TokenType::type));        \
        builder.append(", but got a ");                   \
        builder.append(toString(name##Expected.error())); \
        FAIL(builder.toString());                         \
    }                                                     \
    auto& name = *name##Expected;

#define CONSUME_TYPE(type)                                   \
    do {                                                     \
        auto expectedToken = consumeType(TokenType::type);   \
        if (!expectedToken) {                                \
            StringBuilder builder;                           \
            builder.append("Expected a ");                   \
            builder.append(toString(TokenType::type));       \
            builder.append(", but got a ");                  \
            builder.append(toString(expectedToken.error())); \
            FAIL(builder.toString());                        \
        }                                                    \
    } while (false)

template<typename Lexer>
Result<AST::ShaderModule> parse(const String& wgsl)
{
    Lexer lexer(wgsl);
    Parser parser(lexer);

    return parser.parseShader();
}

Result<AST::ShaderModule> parseLChar(const String& wgsl)
{
    return parse<Lexer<LChar>>(wgsl);
}

Result<AST::ShaderModule> parseUChar(const String& wgsl)
{
    return parse<Lexer<UChar>>(wgsl);
}

template<typename Lexer>
Expected<Token, TokenType> Parser<Lexer>::consumeType(TokenType type)
{
    if (current().m_type == type) {
        Expected<Token, TokenType> result = { m_current };
        m_current = m_lexer.lex();
        return result;
    }
    return makeUnexpected(current().m_type);
}

template<typename Lexer>
void Parser<Lexer>::consume()
{
    m_current = m_lexer.lex();
}

template<typename Lexer>
Result<AST::ShaderModule> Parser<Lexer>::parseShader()
{
    START_PARSE();

    AST::ShaderModule shaderModule;

    // FIXME: parse directives here.
    while (!m_lexer.isAtEndOfFile()) {
        PARSE(attributes, Attributes);

        switch (current().m_type) {
        case TokenType::KeywordStruct: {
            PARSE(structDecl, StructureDeclaration);
            structDecl->attributes() = WTFMove(attributes);
            shaderModule.structures().append(WTFMove(structDecl));
            break;
        }
        case TokenType::KeywordVar: {
            PARSE(variableDecl, VariableDeclaration);
            variableDecl->attributes() = WTFMove(attributes);
            CONSUME_TYPE(Semicolon);
            shaderModule.variables().append(WTFMove(variableDecl));
            break;
        }
        case TokenType::KeywordFn: {
            PARSE(functionDecl, FunctionDeclaration);
            functionDecl->attributes() = WTFMove(attributes);
            shaderModule.functions().append(WTFMove(functionDecl));
            break;
        }
        case TokenType::KeywordType: {
            PARSE(typeDecl, TypeDeclaration);
            typeDecl->attributes() = WTFMove(attributes);
            CONSUME_TYPE(Semicolon);
            shaderModule.types().append(WTFMove(typeDecl));
            break;
        }
        default:
            FAIL("Trying to parse a GlobalDecl, expected 'var', 'fn', or 'struct'."_s);
        }
    }

    return shaderModule;
}

template<typename Lexer>
Result<AST::Attribute::List> Parser<Lexer>::parseAttributes()
{
    AST::Attribute::List attributes;

    while (current().m_type == TokenType::Attribute) {
        PARSE(firstAttribute, Attribute);
        attributes.append(WTFMove(firstAttribute));
    }

    return { WTFMove(attributes) };
}

template<typename Lexer>
Result<AST::Attribute::Ref> Parser<Lexer>::parseAttribute()
{
    START_PARSE();

    CONSUME_TYPE(Attribute);
    CONSUME_TYPE_NAMED(ident, Identifier);

    if (ident.m_ident == "group"_s) {
        CONSUME_TYPE(ParenLeft);
        // FIXME: should more kinds of literals be accepted here?
        CONSUME_TYPE_NAMED(id, IntegerLiteral);
        CONSUME_TYPE(ParenRight);
        RETURN_NODE_REF(GroupAttribute, id.m_literalValue);
    }

    if (ident.m_ident == "binding"_s) {
        CONSUME_TYPE(ParenLeft);
        // FIXME: should more kinds of literals be accepted here?
        CONSUME_TYPE_NAMED(id, IntegerLiteral);
        CONSUME_TYPE(ParenRight);
        RETURN_NODE_REF(BindingAttribute, id.m_literalValue);
    }

    if (ident.m_ident == "location"_s) {
        CONSUME_TYPE(ParenLeft);
        // FIXME: should more kinds of literals be accepted here?
        CONSUME_TYPE_NAMED(id, IntegerLiteral);
        CONSUME_TYPE(ParenRight);
        RETURN_NODE_REF(LocationAttribute, id.m_literalValue);
    }

    if (ident.m_ident == "builtin"_s) {
        CONSUME_TYPE(ParenLeft);
        CONSUME_TYPE_NAMED(name, Identifier);
        CONSUME_TYPE(ParenRight);
        RETURN_NODE_REF(BuiltinAttribute, name.m_ident);
    }

    // https://gpuweb.github.io/gpuweb/wgsl/#pipeline-stage-attributes
    if (ident.m_ident == "vertex"_s)
        RETURN_NODE_REF(StageAttribute, AST::StageAttribute::Stage::Vertex);
    if (ident.m_ident == "compute"_s)
        RETURN_NODE_REF(StageAttribute, AST::StageAttribute::Stage::Compute);
    if (ident.m_ident == "fragment"_s)
        RETURN_NODE_REF(StageAttribute, AST::StageAttribute::Stage::Fragment);

    FAIL("Unknown attribute. Supported attributes are 'group', 'binding', 'location', 'builtin', 'vertex', 'compute', 'fragment'."_s);
}

template<typename Lexer>
Result<AST::StructureDeclaration::Ref> Parser<Lexer>::parseStructureDeclaration()
{
    START_PARSE();

    CONSUME_TYPE(KeywordStruct);
    CONSUME_TYPE_NAMED(name, Identifier);
    CONSUME_TYPE(BraceLeft);

    AST::StructureMember::List members;
    while (current().m_type != TokenType::BraceRight) {
        PARSE(member, StructureMember);
        members.append(makeUniqueRef<AST::StructureMember>(WTFMove(member)));
    }

    CONSUME_TYPE(BraceRight);

    RETURN_NODE_REF(StructureDeclaration, name.m_ident, WTFMove(members));
}

template<typename Lexer>
Result<AST::StructureMember> Parser<Lexer>::parseStructureMember()
{
    START_PARSE();

    PARSE(attributes, Attributes);
    CONSUME_TYPE_NAMED(name, Identifier);
    CONSUME_TYPE(Colon);
    PARSE(type, TypeName);
    CONSUME_TYPE(Semicolon);

    RETURN_NODE(StructureMember, name.m_ident, WTFMove(type), WTFMove(attributes));
}

template<typename Lexer>
Result<AST::TypeName::Ref> Parser<Lexer>::parseTypeName()
{
    START_PARSE();

    if (current().m_type == TokenType::KeywordArray)
        return parseArrayTypeName();
    if (current().m_type == TokenType::KeywordI32) {
        consume();
        RETURN_NODE_REF(NamedTypeName, StringView { "i32"_s });
    }
    if (current().m_type == TokenType::KeywordF32) {
        consume();
        RETURN_NODE_REF(NamedTypeName, StringView { "f32"_s });
    }
    if (current().m_type == TokenType::KeywordU32) {
        consume();
        RETURN_NODE_REF(NamedTypeName, StringView { "u32"_s });
    }
    if (current().m_type == TokenType::KeywordBool) {
        consume();
        RETURN_NODE_REF(NamedTypeName, StringView { "bool"_s });
    }
    if (current().m_type == TokenType::Identifier) {
        CONSUME_TYPE_NAMED(name, Identifier);
        return parseTypeNameAfterIdentifier(WTFMove(name.m_ident), _startOfElementPosition);
    }

    FAIL("Tried parsing a type and it did not start with an identifier"_s);
}

template<typename Lexer>
Result<AST::TypeName::Ref> Parser<Lexer>::parseTypeNameAfterIdentifier(StringView&& name, SourcePosition _startOfElementPosition)
{
    if (auto kind = AST::ParameterizedTypeName::stringViewToKind(name)) {
        CONSUME_TYPE(LT);
        PARSE(elementType, TypeName);
        CONSUME_TYPE(GT);
        RETURN_NODE_REF(ParameterizedTypeName, *kind, WTFMove(elementType));
    }
    RETURN_NODE_REF(NamedTypeName, WTFMove(name));
}

template<typename Lexer>
Result<AST::TypeName::Ref> Parser<Lexer>::parseArrayTypeName()
{
    START_PARSE();

    CONSUME_TYPE(KeywordArray);

    AST::TypeName::Ptr maybeElementType;
    AST::Expression::Ptr maybeElementCount;

    if (current().m_type == TokenType::LT) {
        // We differ from the WGSL grammar here by allowing the type to be optional,
        // which allows us to use `parseArrayType` in `parseCallableExpression`.
        consume();

        PARSE(elementType, TypeName);
        maybeElementType = elementType.moveToUniquePtr();

        if (current().m_type == TokenType::Comma) {
            consume();
            // FIXME: According to https://www.w3.org/TR/WGSL/#syntax-element_count_expression
            // this should be: AdditiveExpression | BitwiseExpression.
            //
            // The WGSL grammar doesn't specify expression operator precedence so
            // until then just parse AdditiveExpression.
            PARSE(elementCount, AdditiveExpression);
            maybeElementCount = elementCount.moveToUniquePtr();
        }
        CONSUME_TYPE(GT);
    }

    RETURN_NODE_REF(ArrayTypeName, WTFMove(maybeElementType), WTFMove(maybeElementCount));
}

template<typename Lexer>
Result<AST::TypeDeclaration::Ref> Parser<Lexer>::parseTypeDeclaration()
{
    START_PARSE();

    CONSUME_TYPE(KeywordType);
    CONSUME_TYPE_NAMED(name, Identifier);
    CONSUME_TYPE(Equal);
    PARSE(typeName, TypeName);

    RETURN_NODE_REF(TypeDeclaration, name.m_ident, WTFMove(typeName));
}

template<typename Lexer>
Result<AST::VariableDeclaration::Ref> Parser<Lexer>::parseVariableDeclaration()
{
    START_PARSE();

    CONSUME_TYPE(KeywordVar);

    AST::VariableQualifier::Ptr maybeQualifier = nullptr;
    if (current().m_type == TokenType::LT) {
        PARSE(variableQualifier, VariableQualifier);
        maybeQualifier = WTF::makeUnique<AST::VariableQualifier>(WTFMove(variableQualifier));
    }

    CONSUME_TYPE_NAMED(name, Identifier);

    AST::TypeName::Ptr maybeType = nullptr;
    if (current().m_type == TokenType::Colon) {
        consume();
        PARSE(typeDecl, TypeName);
        maybeType = typeDecl.moveToUniquePtr();
    }

    AST::Expression::Ptr maybeInitializer = nullptr;
    if (current().m_type == TokenType::Equal) {
        consume();
        PARSE(initializerExpr, Expression);
        maybeInitializer = initializerExpr.moveToUniquePtr();
    }

    RETURN_NODE_REF(VariableDeclaration, name.m_ident, WTFMove(maybeQualifier), WTFMove(maybeType), WTFMove(maybeInitializer));
}

template<typename Lexer>
Result<AST::VariableQualifier> Parser<Lexer>::parseVariableQualifier()
{
    START_PARSE();

    CONSUME_TYPE(LT);
    PARSE(storageClass, StorageClass);

    // FIXME: verify that Read is the correct default in all cases.
    AST::AccessMode accessMode = AST::AccessMode::Read;
    if (current().m_type == TokenType::Comma) {
        consume();
        PARSE(actualAccessMode, AccessMode);
        accessMode = actualAccessMode;
    }

    CONSUME_TYPE(GT);

    RETURN_NODE(VariableQualifier, storageClass, accessMode);
}

template<typename Lexer>
Result<AST::StorageClass> Parser<Lexer>::parseStorageClass()
{
    START_PARSE();

    if (current().m_type == TokenType::KeywordFunction) {
        consume();
        return { AST::StorageClass::Function };
    }
    if (current().m_type == TokenType::KeywordPrivate) {
        consume();
        return { AST::StorageClass::Private };
    }
    if (current().m_type == TokenType::KeywordWorkgroup) {
        consume();
        return { AST::StorageClass::Workgroup };
    }
    if (current().m_type == TokenType::KeywordUniform) {
        consume();
        return { AST::StorageClass::Uniform };
    }
    if (current().m_type == TokenType::KeywordStorage) {
        consume();
        return { AST::StorageClass::Storage };
    }

    FAIL("Expected one of 'function'/'private'/'storage'/'uniform'/'workgroup'"_s);
}

template<typename Lexer>
Result<AST::AccessMode> Parser<Lexer>::parseAccessMode()
{
    START_PARSE();

    if (current().m_type == TokenType::KeywordRead) {
        consume();
        return { AST::AccessMode::Read };
    }
    if (current().m_type == TokenType::KeywordWrite) {
        consume();
        return { AST::AccessMode::Write };
    }
    if (current().m_type == TokenType::KeywordReadWrite) {
        consume();
        return { AST::AccessMode::ReadWrite };
    }

    FAIL("Expected one of 'read'/'write'/'read_write'"_s);
}

template<typename Lexer>
Result<AST::FunctionDeclaration::Ref> Parser<Lexer>::parseFunctionDeclaration()
{
    START_PARSE();

    CONSUME_TYPE(KeywordFn);
    CONSUME_TYPE_NAMED(name, Identifier);

    CONSUME_TYPE(ParenLeft);
    AST::Parameter::List parameters;
    while (current().m_type != TokenType::ParenRight) {
        PARSE(parameter, Parameter);
        parameters.append(makeUniqueRef<AST::Parameter>(WTFMove(parameter)));
    }
    CONSUME_TYPE(ParenRight);

    AST::Attribute::List returnAttributes;
    AST::TypeName::Ptr maybeReturnType = nullptr;
    if (current().m_type == TokenType::Arrow) {
        consume();
        PARSE(parsedReturnAttributes, Attributes);
        returnAttributes = WTFMove(parsedReturnAttributes);
        PARSE(type, TypeName);
        maybeReturnType = type.moveToUniquePtr();
    }

    PARSE(body, CompoundStatement);

    RETURN_NODE_REF(FunctionDeclaration, name.m_ident, WTFMove(parameters), WTFMove(maybeReturnType), WTFMove(body), WTFMove(returnAttributes));
}

template<typename Lexer>
Result<AST::Parameter> Parser<Lexer>::parseParameter()
{
    START_PARSE();

    PARSE(attributes, Attributes);
    CONSUME_TYPE_NAMED(name, Identifier)
    CONSUME_TYPE(Colon);
    PARSE(type, TypeName);

    RETURN_NODE(Parameter, name.m_ident, WTFMove(type), WTFMove(attributes));
}

template<typename Lexer>
Result<AST::Statement::Ref> Parser<Lexer>::parseStatement()
{
    START_PARSE();

    switch (current().m_type) {
    case TokenType::BraceLeft: {
        PARSE(compoundStmt, CompoundStatement);
        return { makeUniqueRef<AST::CompoundStatement>(WTFMove(compoundStmt)) };
    }
    case TokenType::Semicolon: {
        consume();
        AST::Statement::List statements;
        return { makeUniqueRef<AST::CompoundStatement>(CURRENT_SOURCE_SPAN(), WTFMove(statements)) };
    }
    case TokenType::KeywordReturn: {
        PARSE(returnStmt, ReturnStatement);
        CONSUME_TYPE(Semicolon);
        return { makeUniqueRef<AST::ReturnStatement>(WTFMove(returnStmt)) };
    }
    case TokenType::KeywordVar: {
        PARSE(varDecl, VariableDeclaration);
        CONSUME_TYPE(Semicolon);
        return { makeUniqueRef<AST::VariableStatement>(CURRENT_SOURCE_SPAN(), WTFMove(varDecl)) };
    }
    case TokenType::Identifier: {
        // FIXME: there will be other cases here eventually for function calls
        PARSE(lhs, LHSExpression);
        CONSUME_TYPE(Equal);
        PARSE(rhs, Expression);
        CONSUME_TYPE(Semicolon);
        RETURN_NODE_REF(AssignmentStatement, lhs.moveToUniquePtr(), WTFMove(rhs));
    }
    default:
        FAIL("Not a valid statement"_s);
    }
}

template<typename Lexer>
Result<AST::CompoundStatement> Parser<Lexer>::parseCompoundStatement()
{
    START_PARSE();

    CONSUME_TYPE(BraceLeft);

    AST::Statement::List statements;
    while (current().m_type != TokenType::BraceRight) {
        PARSE(stmt, Statement);
        statements.append(WTFMove(stmt));
    }

    CONSUME_TYPE(BraceRight);

    RETURN_NODE(CompoundStatement, WTFMove(statements));
}

template<typename Lexer>
Result<AST::ReturnStatement> Parser<Lexer>::parseReturnStatement()
{
    START_PARSE();

    CONSUME_TYPE(KeywordReturn);

    if (current().m_type == TokenType::Semicolon) {
        RETURN_NODE(ReturnStatement, {});
    }

    PARSE(expr, ShortCircuitOrExpression);
    RETURN_NODE(ReturnStatement, expr.moveToUniquePtr());
}

template<typename Lexer>
Result<AST::Expression::Ref> Parser<Lexer>::parseShortCircuitOrExpression()
{
    // FIXME: fill in
    return parseRelationalExpression();
}

template<typename Lexer>
Result<AST::Expression::Ref> Parser<Lexer>::parseRelationalExpression()
{
    // FIXME: fill in
    return parseShiftExpression();
}

template<typename Lexer>
Result<AST::Expression::Ref> Parser<Lexer>::parseShiftExpression()
{
    // FIXME: fill in
    return parseAdditiveExpression();
}

template<typename Lexer>
Result<AST::Expression::Ref> Parser<Lexer>::parseAdditiveExpression()
{
    // FIXME: fill in
    return parseMultiplicativeExpression();
}

template<typename Lexer>
Result<AST::Expression::Ref> Parser<Lexer>::parseMultiplicativeExpression()
{
    // FIXME: fill in
    return parseUnaryExpression();
}

template<typename Lexer>
Result<AST::Expression::Ref> Parser<Lexer>::parseUnaryExpression()
{
    START_PARSE();

    if (current().m_type == TokenType::Minus) {
        consume();
        PARSE(expression, SingularExpression);
        RETURN_NODE_REF(UnaryExpression, WTFMove(expression), AST::UnaryOperation::Negate);
    }

    return parseSingularExpression();
}

template<typename Lexer>
Result<AST::Expression::Ref> Parser<Lexer>::parseSingularExpression()
{
    START_PARSE();
    PARSE(base, PrimaryExpression);
    return parsePostfixExpression(WTFMove(base), _startOfElementPosition);
}

template<typename Lexer>
Result<AST::Expression::Ref> Parser<Lexer>::parsePostfixExpression(AST::Expression::Ref&& base, SourcePosition startPosition)
{
    START_PARSE();

    AST::Expression::Ref expr = WTFMove(base);
    // FIXME: add the case for array/vector/matrix access

    for (;;) {
        switch (current().m_type) {
        case TokenType::BracketLeft: {
            consume();
            PARSE(arrayIndex, Expression);
            CONSUME_TYPE(BracketRight);
            // FIXME: Replace with NODE_REF(...)
            SourceSpan span(startPosition, m_lexer.currentPosition());
            expr = makeUniqueRef<AST::ArrayAccess>(span, WTFMove(expr), WTFMove(arrayIndex));
            break;
        }

        case TokenType::Period: {
            consume();
            CONSUME_TYPE_NAMED(fieldName, Identifier);
            // FIXME: Replace with NODE_REF(...)
            SourceSpan span(startPosition, m_lexer.currentPosition());
            expr = makeUniqueRef<AST::StructureAccess>(span, WTFMove(expr), fieldName.m_ident);
            break;
        }

        default:
            return { WTFMove(expr) };
        }
    }
}

// https://gpuweb.github.io/gpuweb/wgsl/#syntax-primary_expression
// primary_expression:
//   | ident
//   | callable argument_expression_list
//   | const_literal
//   | paren_expression
//   | bitcast less_than type_decl greater_than paren_expression
template<typename Lexer>
Result<AST::Expression::Ref> Parser<Lexer>::parsePrimaryExpression()
{
    START_PARSE();

    switch (current().m_type) {
    // paren_expression
    case TokenType::ParenLeft: {
        consume();
        PARSE(expr, Expression);
        CONSUME_TYPE(ParenRight);
        return { WTFMove(expr) };
    }
    case TokenType::Identifier: {
        CONSUME_TYPE_NAMED(ident, Identifier);
        if (current().m_type == TokenType::LT || current().m_type == TokenType::ParenLeft) {
            PARSE(type, TypeNameAfterIdentifier, WTFMove(ident.m_ident), _startOfElementPosition);
            PARSE(arguments, ArgumentExpressionList);
            RETURN_NODE_REF(CallableExpression, WTFMove(type), WTFMove(arguments));
        }
        RETURN_NODE_REF(IdentifierExpression, ident.m_ident);
    }
    case TokenType::KeywordArray: {
        PARSE(arrayType, ArrayTypeName);
        PARSE(arguments, ArgumentExpressionList);
        RETURN_NODE_REF(CallableExpression, WTFMove(arrayType), WTFMove(arguments));
    }

    // const_literal
    case TokenType::LiteralTrue:
        consume();
        RETURN_NODE_REF(BoolLiteral, true);
    case TokenType::LiteralFalse:
        consume();
        RETURN_NODE_REF(BoolLiteral, false);
    case TokenType::IntegerLiteral: {
        CONSUME_TYPE_NAMED(lit, IntegerLiteral);
        RETURN_NODE_REF(AbstractIntLiteral, lit.m_literalValue);
    }
    case TokenType::IntegerLiteralSigned: {
        CONSUME_TYPE_NAMED(lit, IntegerLiteralSigned);
        RETURN_NODE_REF(Int32Literal, lit.m_literalValue);
    }
    case TokenType::IntegerLiteralUnsigned: {
        CONSUME_TYPE_NAMED(lit, IntegerLiteralUnsigned);
        RETURN_NODE_REF(Uint32Literal, lit.m_literalValue);
    }
    case TokenType::DecimalFloatLiteral: {
        CONSUME_TYPE_NAMED(lit, DecimalFloatLiteral);
        RETURN_NODE_REF(AbstractFloatLiteral, lit.m_literalValue);
    }
    case TokenType::HexFloatLiteral: {
        CONSUME_TYPE_NAMED(lit, HexFloatLiteral);
        RETURN_NODE_REF(AbstractFloatLiteral, lit.m_literalValue);
    }
        // TODO: bitcast expression

    default:
        break;
    }

    FAIL("Expected one of '(', a literal, or an identifier"_s);
}

template<typename Lexer>
Result<AST::Expression::Ref> Parser<Lexer>::parseExpression()
{
    // FIXME: Fill in
    return parseRelationalExpression();
}

template<typename Lexer>
Result<AST::Expression::Ref> Parser<Lexer>::parseLHSExpression()
{
    START_PARSE();

    // FIXME: Add the possibility of a prefix
    PARSE(base, CoreLHSExpression);
    return parsePostfixExpression(WTFMove(base), _startOfElementPosition);
}

template<typename Lexer>
Result<AST::Expression::Ref> Parser<Lexer>::parseCoreLHSExpression()
{
    START_PARSE();

    switch (current().m_type) {
    case TokenType::ParenLeft: {
        consume();
        PARSE(expr, LHSExpression);
        CONSUME_TYPE(ParenRight);
        return { WTFMove(expr) };
    }
    case TokenType::Identifier: {
        CONSUME_TYPE_NAMED(ident, Identifier);
        RETURN_NODE_REF(IdentifierExpression, ident.m_ident);
    }
    default:
        break;
    }

    FAIL("Tried to parse the left-hand side of an assignment and failed"_s);
}

template<typename Lexer>
Result<AST::Expression::List> Parser<Lexer>::parseArgumentExpressionList()
{
    START_PARSE();
    CONSUME_TYPE(ParenLeft);

    AST::Expression::List arguments;
    while (current().m_type != TokenType::ParenRight) {
        PARSE(expr, Expression);
        arguments.append(WTFMove(expr));
        if (current().m_type != TokenType::ParenRight) {
            CONSUME_TYPE(Comma);
        }
    }

    CONSUME_TYPE(ParenRight);
    return { WTFMove(arguments) };
}

} // namespace WGSL
