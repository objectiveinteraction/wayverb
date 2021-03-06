#pragma once

#include "core/cl/representation.h"
#include "core/cl/traits.h"

namespace wayverb {
namespace waveguide {

constexpr size_t biquad_order{2};
constexpr size_t biquad_sections{3};

////////////////////////////////////////////////////////////////////////////////

using filt_real = cl_double;

////////////////////////////////////////////////////////////////////////////////

/// Just an array of filt_real to use as a delay line.
template <size_t o>
struct alignas(1 << 3) memory final {
    static constexpr size_t order = o;
    filt_real array[order]{};
};

template <size_t D>
inline bool operator==(const memory<D>& a, const memory<D>& b) {
    return std::equal(
            std::begin(a.array), std::end(a.array), std::begin(b.array));
}

template <size_t D>
inline bool operator!=(const memory<D>& a, const memory<D>& b) {
    return !(a == b);
}

////////////////////////////////////////////////////////////////////////////////

/// IIR filter coefficient storage.
template <size_t o>
struct alignas(1 << 3) coefficients final {
    static constexpr auto order = o;
    filt_real b[order + 1]{};
    filt_real a[order + 1]{};
};

template <size_t D>
inline bool operator==(const coefficients<D>& a, const coefficients<D>& b) {
    return std::equal(std::begin(a.a), std::end(a.a), std::begin(b.a)) &&
           std::equal(std::begin(a.b), std::end(a.b), std::begin(b.b));
}

template <size_t D>
inline bool operator!=(const coefficients<D>& a, const coefficients<D>& b) {
    return !(a == b);
}

////////////////////////////////////////////////////////////////////////////////

using memory_biquad = memory<biquad_order>;

using coefficients_biquad = coefficients<biquad_order>;

using memory_canonical = memory<memory_biquad::order * biquad_sections>;

using coefficients_canonical =
        coefficients<coefficients_biquad::order * biquad_sections>;

////////////////////////////////////////////////////////////////////////////////

/// Several biquad delay lines in a row.
struct alignas(1 << 3) biquad_memory_array final {
    memory_biquad array[biquad_sections]{};
};

////////////////////////////////////////////////////////////////////////////////

/// Several sets of biquad parameters.
struct alignas(1 << 3) biquad_coefficients_array final {
    coefficients_biquad array[biquad_sections]{};
};

}  // namespace waveguide

template <>
struct core::cl_representation<waveguide::filt_real> final {
    static constexpr auto value = R"(
typedef double filt_real;
)";
};

template <>
struct core::cl_representation<waveguide::memory_biquad> final {
    static const std::string value;
};

template <>
struct core::cl_representation<waveguide::coefficients_biquad> final {
    static const std::string value;
};

template <>
struct core::cl_representation<waveguide::memory_canonical> final {
    static const std::string value;
};

template <>
struct core::cl_representation<waveguide::coefficients_canonical> final {
    static const std::string value;
};

template <>
struct core::cl_representation<waveguide::biquad_memory_array> final {
    static constexpr auto value = R"(
typedef struct {
    memory_biquad array[BIQUAD_SECTIONS];
} biquad_memory_array;
)";
};

template <>
struct core::cl_representation<waveguide::biquad_coefficients_array> final {
    static constexpr auto value = R"(
typedef struct {
    coefficients_biquad array[BIQUAD_SECTIONS];
} biquad_coefficients_array;
)";
};

}  // namespace wayverb
