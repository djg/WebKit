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
#include "AST/Declarations/NativeTypeDeclaration.h"
#include "AST/Literal.h"
#include "Visitor.h"

#include "AST.h"
#include <wtf/UniqueRef.h>

namespace WGSL {

bool Visitor::hasError() const
{
    return !m_expectedError;
}

Expected<void, Error> Visitor::result()
{
    return m_expectedError;
}

template<typename T> void Visitor::checkErrorAndVisit(T& x)
{
    if (!hasError())
        visit(x);
}

template<typename T> void Visitor::maybeCheckErrorAndVisit(T* x)
{
    if (!hasError() && x)
        visit(*x);
}

#pragma mark -
#pragma mark Shader Module

void Visitor::visit(AST::ShaderModule& shaderModule)
{
    for (auto& directive : shaderModule.directives())
        checkErrorAndVisit(directive);
    for (auto& structureDecl : shaderModule.structures())
        checkErrorAndVisit(structureDecl);
    for (auto& variableDecl : shaderModule.variables())
        checkErrorAndVisit(variableDecl);
    for (auto& functionDecl : shaderModule.functions())
        checkErrorAndVisit(functionDecl);
}

void Visitor::visit(AST::GlobalDirective&)
{
}

#pragma mark -
#pragma mark Attribute

void Visitor::visit(AST::Attribute& attribute)
{
    switch (attribute.kind()) {
    case AST::Attribute::Kind::Binding:
        checkErrorAndVisit(downcast<AST::BindingAttribute>(attribute));
        break;
    case AST::Attribute::Kind::Builtin:
        checkErrorAndVisit(downcast<AST::BuiltinAttribute>(attribute));
        break;
    case AST::Attribute::Kind::Group:
        checkErrorAndVisit(downcast<AST::GroupAttribute>(attribute));
        break;
    case AST::Attribute::Kind::Location:
        checkErrorAndVisit(downcast<AST::LocationAttribute>(attribute));
        break;
    case AST::Attribute::Kind::Native:
        checkErrorAndVisit(downcast<AST::LocationAttribute>(attribute));
        break;
    case AST::Attribute::Kind::Stage:
        checkErrorAndVisit(downcast<AST::StageAttribute>(attribute));
        break;
    }
}

void Visitor::visit(AST::BindingAttribute&)
{
}

void Visitor::visit(AST::BuiltinAttribute&)
{
}

void Visitor::visit(AST::StageAttribute&)
{
}

void Visitor::visit(AST::GroupAttribute&)
{
}

void Visitor::visit(AST::LocationAttribute&)
{
}

void Visitor::visit(AST::NativeAttribute&)
{
}

#pragma mark -
#pragma mark Declaration

void Visitor::visit(AST::Declaration& declaration)
{
    switch (declaration.kind()) {
    case AST::Declaration::Kind::Function:
        checkErrorAndVisit(downcast<AST::FunctionDeclaration>(declaration));
        break;
    case AST::Declaration::Kind::NativeType:
        checkErrorAndVisit(downcast<AST::NativeTypeDeclaration>(declaration));
        break;
    case AST::Declaration::Kind::Structure:
        checkErrorAndVisit(downcast<AST::StructureDeclaration>(declaration));
        break;
    case AST::Declaration::Kind::Type:
        checkErrorAndVisit(downcast<AST::TypeDeclaration>(declaration));
        break;
    case AST::Declaration::Kind::Variable:
        checkErrorAndVisit(downcast<AST::VariableDeclaration>(declaration));
        break;
    }
}

void Visitor::visit(AST::FunctionDeclaration& functionDeclaration)
{
    for (auto& attribute : functionDeclaration.attributes())
        checkErrorAndVisit(attribute);
    for (auto& parameter : functionDeclaration.parameters())
        checkErrorAndVisit(parameter);
    for (auto& attribute : functionDeclaration.returnAttributes())
        checkErrorAndVisit(attribute);
    maybeCheckErrorAndVisit(functionDeclaration.maybeReturnType());
    checkErrorAndVisit(functionDeclaration.body());
}

void Visitor::visit(AST::NativeTypeDeclaration& nativeTypeDeclaration)
{
    for (auto& attribute : nativeTypeDeclaration.attributes())
        checkErrorAndVisit(attribute);
    checkErrorAndVisit(nativeTypeDeclaration.type());
}

void Visitor::visit(AST::StructureDeclaration& structureDeclaration)
{
    for (auto& attribute : structureDeclaration.attributes())
        checkErrorAndVisit(attribute);
    for (auto& member : structureDeclaration.members())
        checkErrorAndVisit(member);
}

void Visitor::visit(AST::TypeDeclaration& typeDeclaration)
{
    for (auto& attribute: typeDeclaration.attributes())
        checkErrorAndVisit(attribute);
}

void Visitor::visit(AST::VariableDeclaration& varDeclaration)
{
    for (auto& attribute : varDeclaration.attributes())
        checkErrorAndVisit(attribute);
    maybeCheckErrorAndVisit(varDeclaration.maybeQualifier());
    maybeCheckErrorAndVisit(varDeclaration.maybeType());
    maybeCheckErrorAndVisit(varDeclaration.maybeInitializer());
}

void Visitor::visit(AST::Parameter& parameter)
{
    for (auto& attribute : parameter.attributes())
        checkErrorAndVisit(attribute);
    checkErrorAndVisit(parameter.type());
}

void Visitor::visit(AST::StructureMember& structureMember)
{
    for (auto& attribute : structureMember.attributes())
        checkErrorAndVisit(attribute);
    checkErrorAndVisit(structureMember.type());
}

void Visitor::visit(AST::VariableQualifier&)
{
}

#pragma mark -
#pragma mark Expression

void Visitor::visit(AST::Expression& expression)
{
    switch (expression.kind()) {
    case AST::Expression::Kind::Literal:
        checkErrorAndVisit(downcast<AST::LiteralExpression>(expression));
        break;
    case AST::Expression::Kind::Identifier:
        checkErrorAndVisit(downcast<AST::IdentifierExpression>(expression));
        break;
    case AST::Expression::Kind::ArrayAccess:
        checkErrorAndVisit(downcast<AST::ArrayAccess>(expression));
        break;
    case AST::Expression::Kind::StructureAccess:
        checkErrorAndVisit(downcast<AST::StructureAccess>(expression));
        break;
    case AST::Expression::Kind::CallableExpression:
        checkErrorAndVisit(downcast<AST::CallableExpression>(expression));
        break;
    case AST::Expression::Kind::UnaryExpression:
        checkErrorAndVisit(downcast<AST::UnaryExpression>(expression));
        break;
    }
}

void Visitor::visit(AST::LiteralExpression& literalExpression)
{
    checkErrorAndVisit(literalExpression.literal());
}

void Visitor::visit(AST::IdentifierExpression&)
{
}

void Visitor::visit(AST::ArrayAccess& arrayAccess)
{
    checkErrorAndVisit(arrayAccess.base());
    checkErrorAndVisit(arrayAccess.index());
}

void Visitor::visit(AST::StructureAccess& structureAccess)
{
    checkErrorAndVisit(structureAccess.base());
}

void Visitor::visit(AST::CallableExpression& callableExpression)
{
    checkErrorAndVisit(callableExpression.target());
    for (auto& argument : callableExpression.arguments())
        checkErrorAndVisit(argument);
}

void Visitor::visit(AST::UnaryExpression& unaryExpression)
{
    checkErrorAndVisit(unaryExpression.expression());
}

#pragma mark -
#pragma mark Literal

void Visitor::visit(AST::Literal& literal)
{
    switch (literal.kind()) {
    case AST::Literal::Kind::Boolean:
        checkErrorAndVisit(downcast<AST::BoolLiteral>(literal));
        break;
    case AST::Literal::Kind::Float:
        checkErrorAndVisit(downcast<AST::FloatLiteral>(literal));
        break;
    case AST::Literal::Kind::Integer:
        checkErrorAndVisit(downcast<AST::IntegerLiteral>(literal));
        break;
    }
}
void Visitor::visit(AST::BoolLiteral&)
{
}

void Visitor::visit(AST::FloatLiteral&)
{
}

void Visitor::visit(AST::IntegerLiteral&)
{
}

#pragma mark -
#pragma mark Statement

void Visitor::visit(AST::Statement& statement)
{
    switch (statement.kind()) {
    case AST::Statement::Kind::Compound:
        checkErrorAndVisit(downcast<AST::CompoundStatement>(statement));
        break;
    case AST::Statement::Kind::Return:
        checkErrorAndVisit(downcast<AST::ReturnStatement>(statement));
        break;
    case AST::Statement::Kind::Assignment:
        checkErrorAndVisit(downcast<AST::AssignmentStatement>(statement));
        break;
    case AST::Statement::Kind::Variable:
        checkErrorAndVisit(downcast<AST::VariableStatement>(statement));
        break;
    }
}

void Visitor::visit(AST::CompoundStatement& compoundStatement)
{
    for (auto& statement : compoundStatement.statements())
        checkErrorAndVisit(statement);
}

void Visitor::visit(AST::ReturnStatement& returnStatement)
{
    maybeCheckErrorAndVisit(returnStatement.maybeExpression());
}

void Visitor::visit(AST::AssignmentStatement& assignStatement)
{
    maybeCheckErrorAndVisit(assignStatement.maybeLhs());
    checkErrorAndVisit(assignStatement.rhs());
}

void Visitor::visit(AST::VariableStatement& varStatement)
{
    checkErrorAndVisit(varStatement.declaration());
}

#pragma mark -
#pragma mark Types

void Visitor::visit(AST::TypeName& TypeName)
{
    switch (TypeName.kind()) {
    case AST::TypeName::Kind::Array:
        checkErrorAndVisit(downcast<AST::ArrayTypeName>(TypeName));
        break;
    case AST::TypeName::Kind::Named:
        checkErrorAndVisit(downcast<AST::NamedTypeName>(TypeName));
        break;
    case AST::TypeName::Kind::Parameterized:
        checkErrorAndVisit(downcast<AST::ParameterizedTypeName>(TypeName));
        break;
    }
}

void Visitor::visit(AST::ArrayTypeName& arrayTypeName)
{
    maybeCheckErrorAndVisit(arrayTypeName.maybeElementType());
    maybeCheckErrorAndVisit(arrayTypeName.maybeElementCount());
}

void Visitor::visit(AST::NamedTypeName&)
{
}

void Visitor::visit(AST::ParameterizedTypeName& parameterizedTypeName)
{
    checkErrorAndVisit(parameterizedTypeName.elementType());
}

} // namespace WGSL
