/*
 * Copyright (C) 2026 Apple Inc. All rights reserved.
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

#include <wtf/FastVariant.h>
#include <wtf/text/WTFString.h>

namespace TestWebKitAPI {

// MARK: - Basic construction

TEST(WTF_FastVariant, DefaultConstruction)
{
    WTF::FastVariant<int, double, String> v;
    EXPECT_EQ(v.index(), 0u);
    EXPECT_TRUE(v.holdsAlternative<int>());
    EXPECT_EQ(WTF::get<int>(v), 0);
}

TEST(WTF_FastVariant, ConvertingConstruction)
{
    WTF::FastVariant<int, double, String> v(42);
    EXPECT_EQ(v.index(), 0u);
    EXPECT_EQ(WTF::get<int>(v), 42);

    WTF::FastVariant<int, double, String> v2(3.14);
    EXPECT_EQ(v2.index(), 1u);
    EXPECT_DOUBLE_EQ(WTF::get<double>(v2), 3.14);
}

TEST(WTF_FastVariant, InPlaceTypeConstruction)
{
    WTF::FastVariant<int, double, String> v(std::in_place_type<double>, 2.718);
    EXPECT_EQ(v.index(), 1u);
    EXPECT_DOUBLE_EQ(WTF::get<double>(v), 2.718);
}

TEST(WTF_FastVariant, InPlaceIndexConstruction)
{
    WTF::FastVariant<int, double, String> v(std::in_place_index<2>, "hello"_s);
    EXPECT_EQ(v.index(), 2u);
    EXPECT_EQ(WTF::get<String>(v), "hello"_s);
}

// MARK: - Copy and move

TEST(WTF_FastVariant, CopyConstruction)
{
    WTF::FastVariant<int, double, String> v(42);
    auto v2 = v;
    EXPECT_EQ(WTF::get<int>(v2), 42);
    EXPECT_EQ(v.index(), v2.index());
}

TEST(WTF_FastVariant, MoveConstruction)
{
    WTF::FastVariant<int, double, String> v("hello"_s);
    auto v2 = WTF::move(v);
    EXPECT_EQ(WTF::get<String>(v2), "hello"_s);
}

TEST(WTF_FastVariant, CopyAssignment)
{
    WTF::FastVariant<int, double, String> v(42);
    WTF::FastVariant<int, double, String> v2(3.14);
    v2 = v;
    EXPECT_EQ(WTF::get<int>(v2), 42);
}

TEST(WTF_FastVariant, MoveAssignment)
{
    WTF::FastVariant<int, double, String> v("hello"_s);
    WTF::FastVariant<int, double, String> v2(42);
    v2 = WTF::move(v);
    EXPECT_EQ(WTF::get<String>(v2), "hello"_s);
}

TEST(WTF_FastVariant, ConvertingAssignment)
{
    WTF::FastVariant<int, double, String> v(42);
    v = 3.14;
    EXPECT_EQ(v.index(), 1u);
    EXPECT_DOUBLE_EQ(WTF::get<double>(v), 3.14);
}

// MARK: - switchOn

TEST(WTF_FastVariant, SwitchOnMember)
{
    WTF::FastVariant<int, double, String> v(42);
    auto result = v.switchOn(
        [](int i) { return i * 2; },
        [](double) { return 0; },
        [](const String&) { return -1; }
    );
    EXPECT_EQ(result, 84);
}

TEST(WTF_FastVariant, SwitchOnVoid)
{
    WTF::FastVariant<int, double> v(42);
    int captured = 0;
    v.switchOn(
        [&](int i) { captured = i; },
        [&](double) { captured = -1; }
    );
    EXPECT_EQ(captured, 42);
}

TEST(WTF_FastVariant, WTFSwitchOn)
{
    WTF::FastVariant<int, double, String> v("world"_s);
    auto result = WTF::switchOn(v,
        [](int) -> String { return "int"_s; },
        [](double) -> String { return "double"_s; },
        [](const String& s) -> String { return s; }
    );
    EXPECT_EQ(result, "world"_s);
}

TEST(WTF_FastVariant, SwitchOnGenericLambda)
{
    // Single auto lambda — the common pattern in WebKit (e.g. TextBreakIteratorCF.h)
    struct A { int value() const { return 1; } };
    struct B { int value() const { return 2; } };
    struct C { int value() const { return 3; } };

    WTF::FastVariant<A, B, C> v(std::in_place_type<B>);
    auto result = v.switchOn([](auto& x) { return x.value(); });
    EXPECT_EQ(result, 2);
}

TEST(WTF_FastVariant, SwitchOnGenericLambdaVoid)
{
    struct A { int val = 1; };
    struct B { int val = 2; };

    WTF::FastVariant<A, B> v(std::in_place_type<A>);
    int captured = 0;
    v.switchOn([&](auto& x) { captured = x.val; });
    EXPECT_EQ(captured, 1);
}

TEST(WTF_FastVariant, SwitchOnReturnsReference)
{
    // Visitor that returns a reference — must not copy.
    struct A { int val = 42; };
    struct B { int val = 99; };

    WTF::FastVariant<A, B> v(std::in_place_type<A>);
    auto& ref = v.switchOn([](auto& x) -> int& { return x.val; });
    EXPECT_EQ(ref, 42);
    ref = 7;
    EXPECT_EQ(WTF::get<A>(v).val, 7);
}

TEST(WTF_FastVariant, SwitchOnSpecificPlusCatchAll)
{
    // Pattern from AXObjectCache.cpp: specific lambdas for some types,
    // catch-all auto& for the rest.
    WTF::FastVariant<int, double, String, float> v(42);
    int which = 0;
    WTF::switchOn(v,
        [&](int i) { which = 1; },
        [&](const String&) { which = 2; },
        [&](auto&) { which = 99; } // catch-all for double, float
    );
    EXPECT_EQ(which, 1); // int matched specifically

    v = 3.14;
    WTF::switchOn(v,
        [&](int) { which = 1; },
        [&](const String&) { which = 2; },
        [&](auto&) { which = 99; }
    );
    EXPECT_EQ(which, 99); // double fell through to catch-all

    v = "hello"_s;
    WTF::switchOn(v,
        [&](int) { which = 1; },
        [&](const String&) { which = 2; },
        [&](auto&) { which = 99; }
    );
    EXPECT_EQ(which, 2); // String matched specifically

    v = 1.5f;
    WTF::switchOn(v,
        [&](int) { which = 1; },
        [&](const String&) { which = 2; },
        [&](auto&) { which = 99; }
    );
    EXPECT_EQ(which, 99); // float fell through to catch-all
}

// MARK: - get / get_if / holds_alternative

TEST(WTF_FastVariant, GetByType)
{
    WTF::FastVariant<int, double, String> v(42);
    EXPECT_EQ(WTF::get<int>(v), 42);
}

TEST(WTF_FastVariant, GetByIndex)
{
    WTF::FastVariant<int, double, String> v(3.14);
    EXPECT_DOUBLE_EQ(WTF::get<1>(v), 3.14);
}

TEST(WTF_FastVariant, StdGetByType)
{
    WTF::FastVariant<int, double, String> v(42);
    EXPECT_EQ(std::get<int>(v), 42);
}

TEST(WTF_FastVariant, StdGetByIndex)
{
    WTF::FastVariant<int, double, String> v(3.14);
    EXPECT_DOUBLE_EQ(std::get<1>(v), 3.14);
}

TEST(WTF_FastVariant, GetIfFound)
{
    WTF::FastVariant<int, double, String> v(42);
    auto* p = WTF::get_if<int>(&v);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 42);
}

TEST(WTF_FastVariant, GetIfNotFound)
{
    WTF::FastVariant<int, double, String> v(42);
    auto* p = WTF::get_if<double>(&v);
    EXPECT_EQ(p, nullptr);
}

TEST(WTF_FastVariant, StdGetIf)
{
    WTF::FastVariant<int, double, String> v(42);
    auto* p = std::get_if<int>(&v);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 42);
}

TEST(WTF_FastVariant, HoldsAlternativeMember)
{
    WTF::FastVariant<int, double, String> v(42);
    EXPECT_TRUE(v.holdsAlternative<int>());
    EXPECT_FALSE(v.holdsAlternative<double>());
}

TEST(WTF_FastVariant, StdHoldsAlternative)
{
    WTF::FastVariant<int, double, String> v(42);
    EXPECT_TRUE(std::holds_alternative<int>(v));
    EXPECT_FALSE(std::holds_alternative<double>(v));
}

TEST(WTF_FastVariant, WTFHoldsAlternative)
{
    WTF::FastVariant<int, double, String> v(42);
    EXPECT_TRUE(WTF::holdsAlternative<int>(v));
    EXPECT_FALSE(WTF::holdsAlternative<double>(v));
}

// MARK: - Comparison

TEST(WTF_FastVariant, EqualSameType)
{
    WTF::FastVariant<int, double> a(42);
    WTF::FastVariant<int, double> b(42);
    EXPECT_TRUE(a == b);
}

TEST(WTF_FastVariant, NotEqualSameType)
{
    WTF::FastVariant<int, double> a(42);
    WTF::FastVariant<int, double> b(43);
    EXPECT_FALSE(a == b);
}

TEST(WTF_FastVariant, NotEqualDifferentType)
{
    WTF::FastVariant<int, double> a(42);
    WTF::FastVariant<int, double> b(42.0);
    EXPECT_FALSE(a == b);
}

// MARK: - Size and traits

TEST(WTF_FastVariant, SizeofIsCompact)
{
    using V = WTF::FastVariant<uint8_t, uint16_t>;
    // Storage: max(1, 2) = 2 bytes, aligned to 2. Plus 1 byte index. Padded to 4.
    EXPECT_LE(sizeof(V), 4u);
}

TEST(WTF_FastVariant, VariantSizeV)
{
    using V = WTF::FastVariant<int, double, String>;
    EXPECT_EQ(WTF::VariantSizeV<V>, 3u);
}

TEST(WTF_FastVariant, StdVariantSize)
{
    using V = WTF::FastVariant<int, double, String>;
    EXPECT_EQ(std::variant_size_v<V>, 3u);
}

// MARK: - Destruction correctness

struct DestructorCounter {
    static int count;
    ~DestructorCounter() { ++count; }
};
int DestructorCounter::count = 0;

TEST(WTF_FastVariant, DestructorCalled)
{
    DestructorCounter::count = 0;
    {
        WTF::FastVariant<int, DestructorCounter> v(std::in_place_type<DestructorCounter>);
    }
    EXPECT_EQ(DestructorCounter::count, 1);
}

TEST(WTF_FastVariant, DestructorCalledOnReassignment)
{
    DestructorCounter::count = 0;
    {
        WTF::FastVariant<int, DestructorCounter> v(std::in_place_type<DestructorCounter>);
        v = 42;
    }
    // One from reassignment, none from final destruction (now int)
    EXPECT_EQ(DestructorCounter::count, 1);
}

// MARK: - Many alternatives (stress test for compile-time)

TEST(WTF_FastVariant, ManyAlternatives)
{
    struct A { int v = 1; bool operator==(const A&) const = default; };
    struct B { int v = 2; bool operator==(const B&) const = default; };
    struct C { int v = 3; bool operator==(const C&) const = default; };
    struct D { int v = 4; bool operator==(const D&) const = default; };
    struct E { int v = 5; bool operator==(const E&) const = default; };
    struct F { int v = 6; bool operator==(const F&) const = default; };
    struct G { int v = 7; bool operator==(const G&) const = default; };
    struct H { int v = 8; bool operator==(const H&) const = default; };

    using V = WTF::FastVariant<A, B, C, D, E, F, G, H>;

    V v(std::in_place_type<E>);
    EXPECT_EQ(v.index(), 4u);

    auto result = v.switchOn(
        [](const A& a) { return a.v; },
        [](const B& b) { return b.v; },
        [](const C& c) { return c.v; },
        [](const D& d) { return d.v; },
        [](const E& e) { return e.v; },
        [](const F& f) { return f.v; },
        [](const G& g) { return g.v; },
        [](const H& h) { return h.v; }
    );
    EXPECT_EQ(result, 5);

    V v2(std::in_place_type<E>);
    EXPECT_TRUE(v == v2);
}

} // namespace TestWebKitAPI
