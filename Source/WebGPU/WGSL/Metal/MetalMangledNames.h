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

#include <wtf/text/ASCIILiteral.h>
#include <wtf/text/StringBuilder.h>

namespace WGSL::Metal {

struct MangledFieldName {
    static constexpr ASCIILiteral prefix = "field"_s;
    unsigned value;
};

struct MangledFunctionName {
    static constexpr ASCIILiteral prefix = "fn"_s;
    unsigned value;
};

struct MangledTypeName {
    static constexpr ASCIILiteral prefix = "type"_s;
    unsigned value;
};

struct MangledVariableName {
    static constexpr ASCIILiteral prefix = "temp"_s;
    unsigned value;
};

} // namespace WGSL::Metal

namespace WTF {

template<typename MangledName>
class MangledTypeAdapter {
public:
    MangledTypeAdapter(MangledName name)
        : m_name(name)
    { }

    bool is8Bit() const { return true; }
    unsigned length() const
    {
        return strlen(MangledName::prefix) + lengthOfIntegerAsString(m_name.value);
    }

    template<typename CharacterType> void writeTo(CharacterType* destination) const
    {
        StringTypeAdapter<ASCIILiteral>(MangledName::prefix).writeTo(destination);
        writeIntegerToBuffer(m_name.value, destination + MangledName::prefix.length());
    }

private:
    MangledName m_name;
};

template<> class StringTypeAdapter<WGSL::Metal::MangledFieldName> : public MangledTypeAdapter<WGSL::Metal::MangledFieldName> {
public:
    StringTypeAdapter(WGSL::Metal::MangledFieldName name) : MangledTypeAdapter(name) {}
};

template<> class StringTypeAdapter<WGSL::Metal::MangledFunctionName> : public MangledTypeAdapter<WGSL::Metal::MangledFunctionName> {
public:
    StringTypeAdapter(WGSL::Metal::MangledFunctionName name) : MangledTypeAdapter(name) {}
};

template<> class StringTypeAdapter<WGSL::Metal::MangledTypeName> : public MangledTypeAdapter<WGSL::Metal::MangledTypeName> {
public:
    StringTypeAdapter(WGSL::Metal::MangledTypeName name) : MangledTypeAdapter(name) {}
};

template<> class StringTypeAdapter<WGSL::Metal::MangledVariableName> : public MangledTypeAdapter<WGSL::Metal::MangledVariableName> {
public:
    StringTypeAdapter(WGSL::Metal::MangledVariableName name) : MangledTypeAdapter(name) {}
};

} // namespace WTF
