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

#include "AST/Attribute.h"
#include "AST/Declaration.h"
#include "AST/Declarations/FunctionDeclaration.h"
#include "AST/Declarations/NativeTypeDeclaration.h"
#include "AST/Declarations/StructureDeclaration.h"
#include "AST/Declarations/TypeDeclaration.h"
#include "AST/Declarations/VariableDeclaration.h"
#include "AST/Expression.h"
#include "AST/Expressions/ArrayAccess.h"
#include "AST/Expressions/CallableExpression.h"
#include "AST/Expressions/IdentifierExpression.h"
#include "AST/Expressions/LiteralExpressions.h"
#include "AST/Expressions/StructureAccess.h"
#include "AST/Expressions/UnaryExpression.h"
#include "AST/GlobalDirective.h"
#include "AST/Literal.h"
#include "AST/ShaderModule.h"
#include "AST/Statement.h"
#include "AST/Statements/AssignmentStatement.h"
#include "AST/Statements/CompoundStatement.h"
#include "AST/Statements/ReturnStatement.h"
#include "AST/Statements/VariableStatement.h"
#include "AST/TypeName.h"
#include "AST/VariableQualifier.h"
