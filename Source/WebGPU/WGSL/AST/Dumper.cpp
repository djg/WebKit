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
#include "Dumper.h"

#include "AST.h"
#include <wtf/DataLog.h>
#include <wtf/EnumTraits.h>
#include <wtf/SetForScope.h>

namespace WGSL::AST {

struct Indent {
    Indent(Dumper& dumper)
    : m_scope(dumper.m_indent, dumper.m_indent + "    ")
    { }
    SetForScope<String> m_scope;
};

static auto bumpIndent(Dumper& dumper) -> Indent
{
    return Indent(dumper);
}

template<typename T, typename J>
void Dumper::visitVector(T& nodes, J joiner)
{
    if (nodes.isEmpty())
        return;

    visit(nodes[0]);
    for (size_t n = 1; n < nodes.size(); ++n) {
        m_out.print(joiner);
        visit(nodes[n]);
    }
}

String Dumper::toString()
{
    return m_out.toString();
}

#define INDENT(...) m_out.print(m_indent __VA_OPT__(,) __VA_ARGS__)
#define NEWLINE()   m_out.print("\n");
#define NL_INDENT() m_out.print("\n", m_indent)
#define NODENT(...) m_out.print(__VA_ARGS__)
#define SPACE()     m_out.print(" ");

// Shader Module
void Dumper::visit(ShaderModule& shaderModule)
{
    for (auto& directive : shaderModule.directives())
        visit(directive);
    if (!shaderModule.directives().isEmpty())
        m_out.print("\n\n");

    for (auto& structure : shaderModule.structures())
        visit(structure);
    if (!shaderModule.structures().isEmpty())
        m_out.printf("\n\n");

    for (auto& variable : shaderModule.variables())
        visit(variable);
    if (!shaderModule.variables().isEmpty())
        m_out.printf("\n\n");

    for (auto& function : shaderModule.functions())
        visit(function);
    if (!shaderModule.functions().isEmpty())
        m_out.printf("\n\n");
}

void Dumper::visit(GlobalDirective& directive)
{
    INDENT("enable ", directive.name(), ";");
}

// Attribute
void Dumper::visit(BindingAttribute& binding)
{
    NODENT("@binding(", binding.binding(), ")");
}

void Dumper::visit(BuiltinAttribute& builtin)
{
    NODENT("@builtin(", builtin.name(), ")");
}

void Dumper::visit(StageAttribute& stage)
{
    switch (stage.stage()) {
    case StageAttribute::Stage::Compute:
        NODENT("@compute");
        break;
    case StageAttribute::Stage::Fragment:
        NODENT("@fragment");
        break;
    case StageAttribute::Stage::Vertex:
        NODENT("@vertex");
        break;
    }
}
void Dumper::visit(GroupAttribute& group)
{
    NODENT("@group(", group.group(), ")");
}

void Dumper::visit(LocationAttribute& location)
{
    NODENT("@location(", location.location(), ")");
}

// Declaration
void Dumper::visit(FunctionDeclaration& function)
{
    INDENT();
    if (!function.attributes().isEmpty()) {
        visitVector(function.attributes(), " ");
        NL_INDENT();
    }
    NODENT("fn ", function.name(), "(");
    if (!function.parameters().isEmpty()) {
        NEWLINE();
        {
            auto indent = bumpIndent(*this);
            visitVector(function.parameters(), "\n");
        }
        NL_INDENT();
    }
    NODENT(")");
    if (function.maybeReturnType()) {
        NODENT(" -> ");
        visitVector(function.returnAttributes(), " ");
        SPACE();
        visit(*function.maybeReturnType());
    }
    NL_INDENT();
    visit(function.body());
}

void Dumper::visit(StructureDeclaration& structure)
{
    INDENT();
    if (!structure.attributes().isEmpty()) {
        visitVector(structure.attributes(), " ");
        NL_INDENT();
    }
    NODENT("struct ", structure.name(), " {");
    if (!structure.members().isEmpty()) {
        NEWLINE();
        {
            auto indent = bumpIndent(*this);
            visitVector(structure.members(), ",\n");
        }
        NL_INDENT();
    }
    NODENT("}\n");
}

void Dumper::visit(VariableDeclaration& variable)
{
    if (!variable.attributes().isEmpty()) {
        visitVector(variable.attributes(), " ");
        NODENT(" ");
    }
    NODENT("var");
    if (variable.maybeQualifier())
        visit(*variable.maybeQualifier());
    NODENT(" ", variable.name());
    if (variable.maybeType()) {
        NODENT(": ");
        visit(*variable.maybeType());
    }
    if (variable.maybeInitializer()) {
        NODENT(" = ");
        visit(*variable.maybeInitializer());
    }
    NODENT(";");
}

void Dumper::visit(Parameter& parameter)
{
    INDENT();
    if (!parameter.attributes().isEmpty()) {
        visitVector(parameter.attributes(), " ");
        SPACE();
    }
    NODENT(parameter.name(), ": ");
    visit(parameter.type());
}

void Dumper::visit(StructureMember& member)
{
    INDENT();
    if (!member.attributes().isEmpty()) {
        visitVector(member.attributes(), " ");
        SPACE();
    }
    NODENT(member.name(), ": ");
    visit(member.type());
}

void Dumper::visit(VariableQualifier& qualifier)
{
    constexpr ASCIILiteral accessMode[]= { "read"_s, "write"_s, "read_write"_s };
    constexpr ASCIILiteral storageClass[] = { "function"_s, "private"_s, "workgroup"_s, "uniform"_s, "storage"_s };
    auto sc = WTF::enumToUnderlyingType(qualifier.storageClass());
    auto am = WTF::enumToUnderlyingType(qualifier.accessMode());
    NODENT("<", storageClass[sc], ",", accessMode[am], ">");
}

// Expression
void Dumper::visit(BoolLiteral& literal)
{
    NODENT(literal.value() ? "true": "false");
}

void Dumper::visit(Int32Literal& literal)
{
    NODENT(literal.value(), "i");
}

void Dumper::visit(Uint32Literal& literal)
{
    NODENT(literal.value(), "u");
}

void Dumper::visit(Float32Literal& literal)
{
    NODENT(literal.value(), "f");
}

void Dumper::visit(AbstractIntLiteral& literal)
{
    NODENT(literal.value());
}

void Dumper::visit(AbstractFloatLiteral& literal)
{
    NODENT(literal.value());
}

void Dumper::visit(IdentifierExpression& identifier)
{
    NODENT(identifier.identifier());
}

void Dumper::visit(ArrayAccess& arrayAccess)
{
    visit(arrayAccess.base());
    NODENT("[");
    visit(arrayAccess.index());
    NODENT("]");
}

void Dumper::visit(StructureAccess& structureAccess)
{
    visit(structureAccess.base());
    NODENT(".", structureAccess.fieldName());
}

void Dumper::visit(CallableExpression& expression)
{
    visit(expression.target());
    NODENT("(");
    if (!expression.arguments().isEmpty()) {
        auto indent = bumpIndent(*this);
        visitVector(expression.arguments(), ", ");
    }
    NODENT(")");
}

void Dumper::visit(UnaryExpression& expression)
{
    constexpr ASCIILiteral unaryOperator[] = { "-"_s };
    auto op = WTF::enumToUnderlyingType(expression.operation());
    NODENT(unaryOperator[op]);
    visit(expression.expression());
}

// Statement
void Dumper::visit(CompoundStatement& block)
{
    INDENT("{");
    if (!block.statements().isEmpty()) {
        {
            auto indent = bumpIndent(*this);
            NEWLINE();
            visitVector(block.statements(), "\n");
        }
        NL_INDENT();
    }
    NODENT("}\n");
}

void Dumper::visit(ReturnStatement& statement)
{
    INDENT("return");
    if (statement.maybeExpression()) {
        SPACE(); visit(*statement.maybeExpression());
    }
    NODENT(";");
}

void Dumper::visit(AssignmentStatement& statement)
{
    INDENT();
    if (statement.maybeLhs()) {
        visit(*statement.maybeLhs());
    } else {
        NODENT("_");
    }
    NODENT(" = ");
    visit(statement.rhs());
    NODENT(";");
}

void Dumper::visit(VariableStatement& statement)
{
    INDENT();
    visit(statement.declaration());
}

// Types
void Dumper::visit(ArrayTypeName& type)
{
    NODENT("array");
    if (type.maybeElementType()) {
        NODENT("<");
        visit(*type.maybeElementType());
        if (type.maybeElementCount()) {
            NODENT(", ");
            visit(*type.maybeElementCount());
        }
        NODENT(">");
    }
}

void Dumper::visit(NamedTypeName& type)
{
    NODENT(type.name());
}

void Dumper::visit(ParameterizedTypeName& type)
{
    constexpr ASCIILiteral base[] = {
        "Vec2"_s,
        "Vec3"_s,
        "Vec4"_s,
        "Mat2x2"_s,
        "Mat2x3"_s,
        "Mat2x4"_s,
        "Mat3x2"_s,
        "Mat3x3"_s,
        "Mat3x4"_s,
        "Mat4x2"_s,
        "Mat4x3"_s,
        "Mat4x4"_s
    };
    auto b = WTF::enumToUnderlyingType(type.base());
    NODENT(base[b], "<");
    visit(type.elementType());
    NODENT(">");
}

template<typename T>
void dumpNode(PrintStream& out, T& node)
{
    Dumper dumper;
    dumper.visit(node);
    out.print(dumper.toString());
}

void dumpAST(ShaderModule& shaderModule)
{
    dataLogLn(ShaderModuleDumper(shaderModule));
}

} // namespace WGSL::AST
