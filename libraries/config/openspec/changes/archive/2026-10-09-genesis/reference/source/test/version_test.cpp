// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// <webcpp/version.hpp> compiles alone, first, and its two macros name one version:
// WEBCPP_LIB_VERSION is "<major>_<minor>", and "_<patch>" after it only when the patch level is
// not 0, as BOOST_LIB_VERSION is to BOOST_VERSION.

#include <webcpp/version.hpp>

#include <array>
#include <string_view>

namespace {

// The numbers of a version as WEBCPP_LIB_VERSION writes it, how many it holds, and whether the
// text was digits separated by single underscores.
struct written {
    std::array<int, 3> numbers{};
    int count = 0;
    bool valid = true;
};

constexpr written read(std::string_view text) {
    written result;
    bool digits = false;
    for (const char c : text) {
        if (c == '_' && digits && result.count < 2) {
            ++result.count;
            digits = false;
        } else if (c >= '0' && c <= '9') {
            result.numbers.at(result.count) = (result.numbers.at(result.count) * 10) + (c - '0');
            digits = true;
        } else {
            result.valid = false;
        }
    }
    result.count += digits ? 1 : 0;
    result.valid = result.valid && digits;
    return result;
}

// Not major and minor: glibc's <sys/sysmacros.h> defines both as macros.
constexpr int version_major = WEBCPP_VERSION / 100000;
constexpr int version_minor = WEBCPP_VERSION / 100 % 1000;
constexpr int version_patch = WEBCPP_VERSION % 100;
constexpr written lib = read(WEBCPP_LIB_VERSION);

static_assert(lib.valid, "WEBCPP_LIB_VERSION is not \"<major>_<minor>[_<patch>]\"");
static_assert(lib.numbers[0] == version_major && lib.numbers[1] == version_minor,
              "WEBCPP_LIB_VERSION names another version than WEBCPP_VERSION");
static_assert(lib.count == (version_patch == 0 ? 2 : 3),
              "WEBCPP_LIB_VERSION writes the patch level exactly when it is not 0");
static_assert(lib.count == 2 || lib.numbers[2] == version_patch,
              "WEBCPP_LIB_VERSION names another patch level than WEBCPP_VERSION");

}  // namespace
