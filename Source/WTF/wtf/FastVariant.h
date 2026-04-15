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

// FastVariant — a compile-time-friendly variant.
//
// Drop-in replacement for WTF::Variant (mpark::variant) that avoids the
// recursive template instantiation patterns that dominate WebCore build time.
//
// Key differences from mpark::variant:
//   - Flat std::byte[] storage instead of recursive union
//   - Fold-expression type-to-index mapping (no recursive traits)
//   - Jump-table visitor dispatch (no recursive dispatcher)
//   - Never valueless — destruction + construction are not interleaved
//   - Maximum 255 alternatives (uint8_t index)

#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <tuple>
#include <utility>
#include <wtf/Assertions.h>
#include <wtf/StdLibExtras.h>

namespace WTF {

namespace FastVariantDetail {

// MARK: - Index-to-type mapping

#if __has_builtin(__type_pack_element)
template <size_t I, typename... Ts>
using TypeAtIndex = __type_pack_element<I, Ts...>;
#else
template <size_t I, typename... Ts>
using TypeAtIndex = typename std::tuple_element<I, std::tuple<Ts...>>::type;
#endif

// MARK: - Type-to-index mapping (fold expression, no recursion)

template <typename T, typename... Ts>
inline constexpr uint8_t indexOfType = [] {
    static_assert((std::is_same_v<T, Ts> || ...), "Type not found in variant alternatives");
    uint8_t i = 0;
    uint8_t result = 0;
    auto matchType = [&](bool isMatch) {
        if (isMatch)
            result = i;
        else
            ++i;
    };
    (matchType(std::is_same_v<T, Ts>), ...);
    return result;
}();

// MARK: - Detecting whether T is one of Ts...

template <typename T, typename... Ts>
inline constexpr bool isOneOf = (std::is_same_v<T, Ts> || ...);

// MARK: - Detecting whether T is an instantiation of a given template

template <typename T, template<typename...> class Template>
inline constexpr bool isOneOfTemplate = false;

template <template<typename...> class Template, typename... Ts>
inline constexpr bool isOneOfTemplate<Template<Ts...>, Template> = true;

// MARK: - Exactly-once check (for converting constructor disambiguation)

template <typename T, typename... Ts>
inline constexpr size_t countOf = (size_t(0) + ... + size_t(std::is_same_v<T, Ts>));

// MARK: - Best match for converting constructor
// Given a value of type T, find which alternative Tj it should convert to.
// This mimics std::variant's overload-resolution-based conversion.

template <typename... Ts>
struct OverloadSet : Ts... {
    using Ts::operator()...;
};

template <typename T>
struct OverloadLeaf {
    T operator()(T) const;
};

template <typename From, typename... Ts>
using BestMatch = typename std::invoke_result_t<OverloadSet<OverloadLeaf<Ts>...>, From>;

template <typename From, typename... Ts>
inline constexpr bool hasBestMatch = requires(From&& from) {
    OverloadSet<OverloadLeaf<Ts>...>{}(std::forward<From>(from));
};

} // namespace FastVariantDetail

// MARK: - FastVariant

template <typename... Ts>
class FastVariant {
    static_assert(sizeof...(Ts) > 0, "FastVariant must have at least one alternative");
    static_assert(sizeof...(Ts) <= 255, "FastVariant supports at most 255 alternatives");

    using Self = FastVariant;

    template <size_t I>
    using Alt = FastVariantDetail::TypeAtIndex<I, Ts...>;

    template <typename T>
    static constexpr uint8_t indexOf = FastVariantDetail::indexOfType<T, Ts...>;

    static constexpr size_t storageSize = std::max({ sizeof(Ts)... });
    static constexpr size_t storageAlign = std::max({ alignof(Ts)... });

    alignas(storageAlign) std::byte m_storage[storageSize];
    uint8_t m_index;

    void* storagePtr() { return m_storage; }
    const void* storagePtr() const { return m_storage; }

    template <typename T>
    T* storageAs() { return std::launder(reinterpret_cast<T*>(m_storage)); }

    template <typename T>
    const T* storageAs() const { return std::launder(reinterpret_cast<const T*>(m_storage)); }

    // MARK: Dispatch helpers (jump table, no recursion)

    template <typename Fn, size_t... Is>
    static void dispatchIndexed(uint8_t index, Fn&& fn, std::index_sequence<Is...>)
    {
        using FnPtr = void(*)(Fn&&);
        static constexpr FnPtr table[] = {
            +[](Fn&& f) { std::forward<Fn>(f).template operator()<Is>(); }...
        };
        WTF_ALLOW_UNSAFE_BUFFER_USAGE_BEGIN
        table[index](std::forward<Fn>(fn));
        WTF_ALLOW_UNSAFE_BUFFER_USAGE_END
    }

    template <typename Fn>
    void dispatch(Fn&& fn)
    {
        dispatchIndexed(m_index, std::forward<Fn>(fn), std::index_sequence_for<Ts...>{});
    }

    template <typename Fn>
    void dispatch(Fn&& fn) const
    {
        dispatchIndexed(m_index, std::forward<Fn>(fn), std::index_sequence_for<Ts...>{});
    }

    // MARK: Destroy

    void destroy()
    {
        auto* storage = storagePtr();
        dispatch([storage]<size_t I>() {
            static_cast<Alt<I>*>(storage)->~Alt<I>();
        });
    }

    // MARK: Copy-construct from another FastVariant's storage

    void copyFrom(const FastVariant& other)
    {
        m_index = other.m_index;
        other.dispatch([&]<size_t I>() {
            new (storagePtr()) Alt<I>(*other.template storageAs<Alt<I>>());
        });
    }

    // MARK: Move-construct from another FastVariant's storage

    void moveFrom(FastVariant&& other)
    {
        m_index = other.m_index;
        other.dispatch([&]<size_t I>() {
            new (storagePtr()) Alt<I>(WTF::move(*other.template storageAs<Alt<I>>()));
        });
    }

public:
    // MARK: - Construction

    FastVariant()
        requires std::is_default_constructible_v<Alt<0>>
    {
        m_index = 0;
        new (storagePtr()) Alt<0>();
    }

    FastVariant(const FastVariant& other)
    {
        copyFrom(other);
    }

    FastVariant(FastVariant&& other) noexcept
    {
        moveFrom(WTF::move(other));
    }

    // Converting constructor: T -> best matching alternative
    template <typename T>
        requires (
            !std::is_same_v<std::remove_cvref_t<T>, FastVariant>
            && FastVariantDetail::hasBestMatch<T, Ts...>
        )
    FastVariant(T&& value)
    {
        using Target = FastVariantDetail::BestMatch<T, Ts...>;
        m_index = indexOf<Target>;
        new (storagePtr()) Target(std::forward<T>(value));
    }

    // In-place type construction
    template <typename T, typename... Args>
        requires FastVariantDetail::isOneOf<T, Ts...>
    explicit FastVariant(std::in_place_type_t<T>, Args&&... args)
    {
        m_index = indexOf<T>;
        new (storagePtr()) T(std::forward<Args>(args)...);
    }

    // In-place index construction
    template <size_t I, typename... Args>
        requires (I < sizeof...(Ts))
    explicit FastVariant(std::in_place_index_t<I>, Args&&... args)
    {
        m_index = I;
        new (storagePtr()) Alt<I>(std::forward<Args>(args)...);
    }

    ~FastVariant()
    {
        destroy();
    }

    // MARK: - Assignment

    FastVariant& operator=(const FastVariant& other)
    {
        if (this != &other) {
            destroy();
            copyFrom(other);
        }
        return *this;
    }

    FastVariant& operator=(FastVariant&& other) noexcept
    {
        if (this != &other) {
            destroy();
            moveFrom(WTF::move(other));
        }
        return *this;
    }

    // Converting assignment
    template <typename T>
        requires (
            !std::is_same_v<std::remove_cvref_t<T>, FastVariant>
            && FastVariantDetail::hasBestMatch<T, Ts...>
        )
    FastVariant& operator=(T&& value)
    {
        using Target = FastVariantDetail::BestMatch<T, Ts...>;
        destroy();
        m_index = indexOf<Target>;
        new (storagePtr()) Target(std::forward<T>(value));
        return *this;
    }

    // MARK: - Inspection

    constexpr size_t index() const noexcept { return m_index; }

    template <typename T>
    bool holdsAlternative() const
    {
        return m_index == indexOf<T>;
    }

    // MARK: - Visitation (member function — picked up by HasSwitchOn concept)

    template <typename... F>
    ALWAYS_INLINE decltype(auto) switchOn(F&&... f) const
    {
        auto visitor = makeVisitor(std::forward<F>(f)...);
        return switchOnImpl(*this, WTF::move(visitor), std::index_sequence_for<Ts...>{});
    }

    template <typename... F>
    ALWAYS_INLINE decltype(auto) switchOn(F&&... f)
    {
        auto visitor = makeVisitor(std::forward<F>(f)...);
        return switchOnImpl(*this, WTF::move(visitor), std::index_sequence_for<Ts...>{});
    }

    // MARK: - Comparison

    bool operator==(const FastVariant& other) const
    {
        if (m_index != other.m_index)
            return false;
        bool result = false;
        dispatch([&]<size_t I>() {
            result = *storageAs<Alt<I>>() == *other.template storageAs<Alt<I>>();
        });
        return result;
    }

private:
    // MARK: - switchOn implementation (jump table)

    template <typename SelfRef, typename Visitor, size_t... Is>
    static ALWAYS_INLINE decltype(auto) switchOnImpl(SelfRef&& self, Visitor&& vis, std::index_sequence<Is...>)
    {
        // Deduce return type from the first alternative. All alternatives must
        // return the same type for switchOn to compile.
        using R = decltype(std::forward<Visitor>(vis)(*std::forward<SelfRef>(self).template storageAs<Alt<0>>()));

        if constexpr (std::is_void_v<std::remove_cvref_t<R>>) {
            using FnPtr = void(*)(Visitor&, void*);
            static constexpr FnPtr table[] = {
                +[](Visitor& v, void* s) {
                    v(*static_cast<Alt<Is>*>(s));
                }...
            };
            WTF_ALLOW_UNSAFE_BUFFER_USAGE_BEGIN
            table[self.m_index](vis, const_cast<void*>(self.storagePtr()));
            WTF_ALLOW_UNSAFE_BUFFER_USAGE_END
        } else if constexpr (std::is_reference_v<R>) {
            // Reference return: use pointer indirection to avoid copies.
            using Pointee = std::remove_reference_t<R>;
            using FnPtr = Pointee*(*)(Visitor&, void*);
            static constexpr FnPtr table[] = {
                +[](Visitor& v, void* s) -> Pointee* {
                    return &v(*static_cast<Alt<Is>*>(s));
                }...
            };
            WTF_ALLOW_UNSAFE_BUFFER_USAGE_BEGIN
            return static_cast<R>(*table[self.m_index](vis, const_cast<void*>(self.storagePtr())));
            WTF_ALLOW_UNSAFE_BUFFER_USAGE_END
        } else {
            using FnPtr = R(*)(Visitor&, void*);
            static constexpr FnPtr table[] = {
                +[](Visitor& v, void* s) -> R {
                    return v(*static_cast<Alt<Is>*>(s));
                }...
            };
            WTF_ALLOW_UNSAFE_BUFFER_USAGE_BEGIN
            return table[self.m_index](vis, const_cast<void*>(self.storagePtr()));
            WTF_ALLOW_UNSAFE_BUFFER_USAGE_END
        }
    }

    // MARK: - Friend access for get/get_if

    template <typename T, typename... Us> friend T& get(FastVariant<Us...>&);
    template <typename T, typename... Us> friend const T& get(const FastVariant<Us...>&);
    template <typename T, typename... Us> friend T&& get(FastVariant<Us...>&&);
    template <typename T, typename... Us> friend const T&& get(const FastVariant<Us...>&&);
    template <size_t I, typename... Us> friend auto& get(FastVariant<Us...>&);
    template <size_t I, typename... Us> friend const auto& get(const FastVariant<Us...>&);
    template <size_t I, typename... Us> friend auto&& get(FastVariant<Us...>&&);
    template <size_t I, typename... Us> friend const auto&& get(const FastVariant<Us...>&&);
    template <typename T, typename... Us> friend std::add_pointer_t<T> get_if(FastVariant<Us...>*) noexcept;
    template <typename T, typename... Us> friend std::add_pointer_t<const T> get_if(const FastVariant<Us...>*) noexcept;
    template <size_t I, typename... Us> friend auto* get_if(FastVariant<Us...>*) noexcept;
    template <size_t I, typename... Us> friend const auto* get_if(const FastVariant<Us...>*) noexcept;
};

// MARK: - Free functions: get<T>

template <typename T, typename... Ts>
T& get(FastVariant<Ts...>& v)
{
    RELEASE_ASSERT(v.template holdsAlternative<T>());
    return *v.template storageAs<T>();
}

template <typename T, typename... Ts>
const T& get(const FastVariant<Ts...>& v)
{
    RELEASE_ASSERT(v.template holdsAlternative<T>());
    return *v.template storageAs<T>();
}

template <typename T, typename... Ts>
T&& get(FastVariant<Ts...>&& v)
{
    RELEASE_ASSERT(v.template holdsAlternative<T>());
    return WTF::move(*v.template storageAs<T>());
}

template <typename T, typename... Ts>
const T&& get(const FastVariant<Ts...>&& v)
{
    RELEASE_ASSERT(v.template holdsAlternative<T>());
    return WTF::move(*v.template storageAs<T>());
}

// MARK: - Free functions: get<I>

template <size_t I, typename... Ts>
auto& get(FastVariant<Ts...>& v)
{
    using T = FastVariantDetail::TypeAtIndex<I, Ts...>;
    RELEASE_ASSERT(v.index() == I);
    return *v.template storageAs<T>();
}

template <size_t I, typename... Ts>
const auto& get(const FastVariant<Ts...>& v)
{
    using T = FastVariantDetail::TypeAtIndex<I, Ts...>;
    RELEASE_ASSERT(v.index() == I);
    return *v.template storageAs<T>();
}

template <size_t I, typename... Ts>
auto&& get(FastVariant<Ts...>&& v)
{
    using T = FastVariantDetail::TypeAtIndex<I, Ts...>;
    RELEASE_ASSERT(v.index() == I);
    return WTF::move(*v.template storageAs<T>());
}

template <size_t I, typename... Ts>
const auto&& get(const FastVariant<Ts...>&& v)
{
    using T = FastVariantDetail::TypeAtIndex<I, Ts...>;
    RELEASE_ASSERT(v.index() == I);
    return WTF::move(*v.template storageAs<T>());
}

// MARK: - Free functions: get_if<T>

template <typename T, typename... Ts>
std::add_pointer_t<T> get_if(FastVariant<Ts...>* v) noexcept
{
    if (!v || !v->template holdsAlternative<T>())
        return nullptr;
    return v->template storageAs<T>();
}

template <typename T, typename... Ts>
std::add_pointer_t<const T> get_if(const FastVariant<Ts...>* v) noexcept
{
    if (!v || !v->template holdsAlternative<T>())
        return nullptr;
    return v->template storageAs<T>();
}

// MARK: - Free functions: get_if<I>

template <size_t I, typename... Ts>
auto* get_if(FastVariant<Ts...>* v) noexcept
{
    using T = FastVariantDetail::TypeAtIndex<I, Ts...>;
    if (!v || v->index() != I)
        return static_cast<T*>(nullptr);
    return v->template storageAs<T>();
}

template <size_t I, typename... Ts>
const auto* get_if(const FastVariant<Ts...>* v) noexcept
{
    using T = FastVariantDetail::TypeAtIndex<I, Ts...>;
    if (!v || v->index() != I)
        return static_cast<const T*>(nullptr);
    return v->template storageAs<T>();
}

// MARK: - Free function: holds_alternative

template <typename T, typename... Ts>
bool holds_alternative(const FastVariant<Ts...>& v)
{
    return v.template holdsAlternative<T>();
}

// MARK: - visit (single FastVariant)

template <typename Visitor, typename... Ts>
ALWAYS_INLINE decltype(auto) visit(Visitor&& visitor, FastVariant<Ts...>& value)
{
    return value.switchOn(std::forward<Visitor>(visitor));
}

template <typename Visitor, typename... Ts>
ALWAYS_INLINE decltype(auto) visit(Visitor&& visitor, const FastVariant<Ts...>& value)
{
    return value.switchOn(std::forward<Visitor>(visitor));
}

template <typename Visitor, typename... Ts>
ALWAYS_INLINE decltype(auto) visit(Visitor&& visitor, FastVariant<Ts...>&& value)
{
    return WTF::move(value).switchOn(std::forward<Visitor>(visitor));
}

// MARK: - visit (two FastVariants — nested switchOn dispatch)

template <typename Visitor, typename V1, typename V2>
    requires (FastVariantDetail::isOneOfTemplate<std::remove_cvref_t<V1>, FastVariant>
           && FastVariantDetail::isOneOfTemplate<std::remove_cvref_t<V2>, FastVariant>)
constexpr decltype(auto) visit(Visitor&& visitor, V1&& a, V2&& b)
{
    return std::forward<V1>(a).switchOn([&](auto&& aVal) -> decltype(auto) {
        return std::forward<V2>(b).switchOn([&](auto&& bVal) -> decltype(auto) {
            return std::forward<Visitor>(visitor)(
                std::forward<decltype(aVal)>(aVal),
                std::forward<decltype(bVal)>(bVal));
        });
    });
}

} // namespace WTF

using WTF::FastVariant;

// MARK: - std namespace overloads (for FORWARD_VARIANT_FUNCTIONS compat)

namespace std {

template <typename T, typename... Ts>
T& get(WTF::FastVariant<Ts...>& v) { return WTF::get<T>(v); }

template <typename T, typename... Ts>
const T& get(const WTF::FastVariant<Ts...>& v) { return WTF::get<T>(v); }

template <typename T, typename... Ts>
T&& get(WTF::FastVariant<Ts...>&& v) { return WTF::get<T>(WTF::move(v)); }

template <typename T, typename... Ts>
const T&& get(const WTF::FastVariant<Ts...>&& v) { return WTF::get<T>(WTF::move(v)); }

template <size_t I, typename... Ts>
auto& get(WTF::FastVariant<Ts...>& v) { return WTF::get<I>(v); }

template <size_t I, typename... Ts>
const auto& get(const WTF::FastVariant<Ts...>& v) { return WTF::get<I>(v); }

template <size_t I, typename... Ts>
auto&& get(WTF::FastVariant<Ts...>&& v) { return WTF::get<I>(WTF::move(v)); }

template <size_t I, typename... Ts>
const auto&& get(const WTF::FastVariant<Ts...>&& v) { return WTF::get<I>(WTF::move(v)); }

template <typename T, typename... Ts>
add_pointer_t<T> get_if(WTF::FastVariant<Ts...>* v) noexcept { return WTF::get_if<T>(v); }

template <typename T, typename... Ts>
add_pointer_t<const T> get_if(const WTF::FastVariant<Ts...>* v) noexcept { return WTF::get_if<T>(v); }

template <size_t I, typename... Ts>
auto* get_if(WTF::FastVariant<Ts...>* v) noexcept { return WTF::get_if<I>(v); }

template <size_t I, typename... Ts>
const auto* get_if(const WTF::FastVariant<Ts...>* v) noexcept { return WTF::get_if<I>(v); }

template <typename T, typename... Ts>
bool holds_alternative(const WTF::FastVariant<Ts...>& v) noexcept { return v.template holdsAlternative<T>(); }

template <typename... Ts>
struct variant_size<WTF::FastVariant<Ts...>> : integral_constant<size_t, sizeof...(Ts)> { };

template <size_t I, typename... Ts>
struct variant_alternative<I, WTF::FastVariant<Ts...>> {
    using type = WTF::FastVariantDetail::TypeAtIndex<I, Ts...>;
};

} // namespace std

