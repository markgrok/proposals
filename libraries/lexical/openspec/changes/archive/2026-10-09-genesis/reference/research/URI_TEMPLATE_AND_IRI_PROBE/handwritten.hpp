// Prototype: RFC 6570 template validation and RFC 3987 IRI / IRI-reference validation.
// No exceptions, no allocation; IPv6 literals delegate to Boost.URL.
#pragma once
#include <boost/url/ipv6_address.hpp>
#include <string_view>

namespace hw {
using sv = std::string_view;

constexpr bool alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
constexpr bool digit(char c) { return c >= '0' && c <= '9'; }
constexpr bool hex(char c) { return digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
constexpr bool unreserved(char c) { return alpha(c) || digit(c) || c == '-' || c == '.' || c == '_' || c == '~'; }
constexpr bool sub_delim(char c) { return sv("!$&'()*+,;=").find(c) != sv::npos; }

// RFC 3629 strict decoding: rejects truncation, overlongs, surrogates and values above U+10FFFF.
constexpr bool decode(sv s, std::size_t& i, char32_t& cp) {
    const auto b = static_cast<unsigned char>(s[i]);
    std::size_t n = b >= 0xF0 ? 4 : b >= 0xE0 ? 3 : b >= 0xC2 ? 2 : 0;
    if (n == 0 || b > 0xF4 || i + n > s.size()) return false;
    cp = b & (0x7F >> n);
    for (std::size_t k = 1; k < n; ++k) {
        const auto c = static_cast<unsigned char>(s[i + k]);
        if ((c & 0xC0) != 0x80) return false;
        cp = (cp << 6) | (c & 0x3F);
    }
    constexpr char32_t min[] = {0, 0, 0x80, 0x800, 0x10000};
    if (cp < min[n] || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return false;
    i += n;
    return true;
}
constexpr bool ucschar(char32_t c) {
    if (c >= 0xA0 && c <= 0xD7FF) return true;
    if (c >= 0xF900 && c <= 0xFDCF) return true;
    if (c >= 0xFDF0 && c <= 0xFFEF) return true;
    if (c >= 0xE0000 && c < 0xE1000) return false;
    return c >= 0x10000 && c <= 0xEFFFD && (c & 0xFFFF) <= 0xFFFD;
}
constexpr bool iprivate(char32_t c) {
    return (c >= 0xE000 && c <= 0xF8FF) || (c >= 0xF0000 && c <= 0xFFFFD) || (c >= 0x100000 && c <= 0x10FFFD);
}
// RFC 3987 4.1: an IRI MUST NOT contain LRM, RLM, LRE, RLE, PDF, LRO or RLO.
constexpr bool bidi_format(char32_t c) { return c == 0x200E || c == 0x200F || (c >= 0x202A && c <= 0x202E); }

// One run of characters: the given ASCII set, pct-encoded, ucschar, and iprivate when allowed.
template <class Ascii> bool run(sv s, Ascii ascii, bool private_use) {
    for (std::size_t i = 0; i < s.size();) {
        const char c = s[i];
        if (c == '%') {
            if (i + 2 >= s.size()) return false;
            if (!hex(s[i + 1]) || !hex(s[i + 2])) return false;
            i += 3;
        } else if (static_cast<unsigned char>(c) < 0x80) {
            if (!ascii(c)) return false;
            ++i;
        } else {
            char32_t cp = 0;
            if (!decode(s, i, cp) || bidi_format(cp)) return false;
            if (!ucschar(cp) && !(private_use && iprivate(cp))) return false;
        }
    }
    return true;
}
constexpr auto pchar = [](char c) { return unreserved(c) || sub_delim(c) || c == ':' || c == '@'; };
constexpr auto path_char = [](char c) { return pchar(c) || c == '/'; };
constexpr auto query_char = [](char c) { return pchar(c) || c == '/' || c == '?'; };

inline bool port(sv p) {
    for (char c : p) if (!digit(c)) return false;
    return true;
}
inline bool ip_literal(sv lit) {
    if (!lit.empty() && (lit[0] == 'v' || lit[0] == 'V')) {  // IPvFuture
        auto dot = lit.find('.');
        if (dot == sv::npos || dot == 1 || dot + 1 == lit.size()) return false;
        for (std::size_t i = 1; i < dot; ++i) if (!hex(lit[i])) return false;
        for (char c : lit.substr(dot + 1)) if (!unreserved(c) && !sub_delim(c) && c != ':') return false;
        return true;
    }
    return boost::urls::parse_ipv6_address(lit).has_value();
}
inline bool authority(sv a) {
    if (auto at = a.find('@'); at != sv::npos) {
        if (!run(a.substr(0, at), [](char c) { return unreserved(c) || sub_delim(c) || c == ':'; }, false)) return false;
        a.remove_prefix(at + 1);
    }
    if (!a.empty() && a[0] == '[') {
        auto close = a.find(']');
        if (close == sv::npos || !ip_literal(a.substr(1, close - 1))) return false;
        sv after = a.substr(close + 1);
        return after.empty() || (after[0] == ':' && port(after.substr(1)));
    }
    auto colon = a.find(':');
    if (colon != sv::npos && !port(a.substr(colon + 1))) return false;
    return run(a.substr(0, colon), [](char c) { return unreserved(c) || sub_delim(c); }, false);
}
inline bool scheme(sv s) {
    if (s.empty() || !alpha(s[0])) return false;
    for (char c : s) if (!alpha(c) && !digit(c) && c != '+' && c != '-' && c != '.') return false;
    return true;
}
// IRI (require_scheme) or IRI-reference. Components are split as RFC 3986 appendix B does,
// then each is checked against its own rule, so no backtracking is needed.
inline bool iri(sv s, bool require_scheme) {
    sv rest = s;
    if (auto h = rest.find('#'); h != sv::npos) {
        if (!run(rest.substr(h + 1), query_char, false)) return false;
        rest = rest.substr(0, h);
    }
    if (auto q = rest.find('?'); q != sv::npos) {
        if (!run(rest.substr(q + 1), query_char, true)) return false;
        rest = rest.substr(0, q);
    }
    if (auto c = rest.find_first_of(":/"); c != sv::npos && rest[c] == ':') {
        if (!scheme(rest.substr(0, c))) return false;  // a colon in a relative path's first segment
        rest.remove_prefix(c + 1);
    } else if (require_scheme) {
        return false;
    }
    if (rest.starts_with("//")) {
        auto slash = rest.find('/', 2);
        if (!authority(rest.substr(2, slash == sv::npos ? sv::npos : slash - 2))) return false;
        rest = slash == sv::npos ? sv{} : rest.substr(slash);
    }
    return run(rest, path_char, false);
}

// RFC 6570 section 2, with erratum 6937 (an apostrophe is a literal).
inline bool uri_template(sv s) {
    auto literal = [](char c) {
        auto u = static_cast<unsigned char>(c);
        return u == 0x21 || u == 0x23 || u == 0x24 || (u >= 0x26 && u <= 0x3B) || u == 0x3D ||
               (u >= 0x3F && u <= 0x5B) || u == 0x5D || u == 0x5F || (u >= 0x61 && u <= 0x7A) || u == 0x7E;
    };
    auto pct = [&](std::size_t& i) {
        if (i + 2 >= s.size() || !hex(s[i + 1]) || !hex(s[i + 2])) return false;
        i += 3;
        return true;
    };
    auto varchar = [&](std::size_t& i) {
        if (i >= s.size()) return false;
        if (s[i] == '%') return pct(i);
        if (alpha(s[i]) || digit(s[i]) || s[i] == '_') return ++i, true;
        return false;
    };
    for (std::size_t i = 0; i < s.size();) {
        const char c = s[i];
        if (c == '{') {
            ++i;
            if (i < s.size() && sv("+#./;?&=,!@|").find(s[i]) != sv::npos) ++i;
            for (;;) {
                if (!varchar(i)) return false;
                while (i < s.size()) {
                    if (s[i] == '.') { ++i; if (!varchar(i)) return false; }
                    else if (!varchar(i)) break;
                }
                if (i < s.size() && s[i] == ':') {
                    ++i;
                    if (i >= s.size() || s[i] < '1' || s[i] > '9') return false;
                    std::size_t start = i++;
                    while (i < s.size() && digit(s[i])) ++i;
                    if (i - start > 4) return false;
                } else if (i < s.size() && s[i] == '*') {
                    ++i;
                }
                if (i < s.size() && s[i] == ',') { ++i; continue; }
                if (i < s.size() && s[i] == '}') { ++i; break; }
                return false;
            }
        } else if (c == '%') {
            if (!pct(i)) return false;
        } else if (static_cast<unsigned char>(c) < 0x80) {
            if (!literal(c)) return false;
            ++i;
        } else {
            char32_t cp = 0;
            if (!decode(s, i, cp) || !(ucschar(cp) || iprivate(cp))) return false;
        }
    }
    return true;
}
}  // namespace hw
