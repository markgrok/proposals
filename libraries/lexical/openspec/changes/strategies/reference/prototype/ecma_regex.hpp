// A prototype: ECMA-262 (u flag) regular expressions on Boost.Regex 1.92, without patching it.
//
// Three extension points of Boost.Regex carry it:
//   - ecma::traits, a char32_t traits class (Boost's "Traits Class Requirements" plus
//     boost_extensions_tag) whose char_class_type is a 512-bit mask, backed by the generated
//     Unicode 17 tables of ucd_tables.hpp: General_Category, Script, Script_Extensions, the 53
//     binary properties ECMA-262 lists, simple case folding. No ICU, no std::locale.
//   - ecma::translate, an ECMA-262 Pattern[+UnicodeMode] parser that refuses what ECMA-262
//     refuses and writes the Boost perl-syntax pattern with ECMA-262's semantics, over code points.
//   - ecma::u8_iterator, a bidirectional UTF-8 iterator with a step budget: when the budget runs
//     out it compares equal to every other iterator of the search, so the matcher believes it is
//     at the end of the input everywhere and stops; search() then answers `limit`.
#pragma once

#include "ucd_tables.hpp"

#include <boost/regex.hpp>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace ecma {

// The char_class_type: one bit per class (ucd_tables.hpp numbers them). Boost ORs the classes of
// a set into one mask and asks isctype(c, mask), so a mask means "c is in any of these".
struct class_mask {
    std::uint64_t w[8] = {};
    constexpr class_mask() = default;
    constexpr class_mask(int) {}  // Boost writes `m = 0` and `m != 0`
    static constexpr class_mask bit(unsigned b) {
        class_mask m;
        m.w[b / 64] = std::uint64_t(1) << (b % 64);
        return m;
    }
    constexpr class_mask& operator|=(class_mask const& o) {
        for (int i = 0; i < 8; ++i) w[i] |= o.w[i];
        return *this;
    }
    friend constexpr class_mask operator|(class_mask a, class_mask const& b) { return a |= b; }
    friend constexpr class_mask operator&(class_mask a, class_mask const& b) {
        for (int i = 0; i < 8; ++i) a.w[i] &= b.w[i];
        return a;
    }
    friend constexpr bool operator==(class_mask const& a, class_mask const& b) {
        for (int i = 0; i < 8; ++i)
            if (a.w[i] != b.w[i]) return false;
        return true;
    }
};

namespace detail {

enum : unsigned { bit_w = 0, bit_s = 1, bit_d = 2, bit_lower = 3, bit_upper = 4, bit_alpha = 5, bit_any = 6 };

template <class Run>
inline unsigned run_value(Run const* runs, std::size_t n, char32_t c) {
    std::size_t lo = 0, hi = n;  // the last run whose start <= c
    while (hi - lo > 1) {
        std::size_t mid = (lo + hi) / 2;
        if (runs[mid].start <= c) lo = mid; else hi = mid;
    }
    return runs[lo].value;
}
inline unsigned general_category(char32_t c) {
    return run_value(ecma_ucd::gc_runs, std::size(ecma_ucd::gc_runs), c);
}
inline unsigned script(char32_t c) { return run_value(ecma_ucd::sc_runs, std::size(ecma_ucd::sc_runs), c); }
inline bool in_binary(unsigned prop, char32_t c) {
    auto first = ecma_ucd::bin_ranges + ecma_ucd::bin_offsets[prop];
    auto last = ecma_ucd::bin_ranges + ecma_ucd::bin_offsets[prop + 1];
    auto it = std::upper_bound(first, last, c, [](char32_t v, ecma_ucd::range const& r) { return v < r.lo; });
    return it != first && c <= (it - 1)->hi;
}
inline bool in_script_extensions(unsigned sc, char32_t c) {
    std::size_t lo = 0, hi = std::size(ecma_ucd::scx_runs);
    while (hi - lo > 1) {
        std::size_t mid = (lo + hi) / 2;
        if (ecma_ucd::scx_runs[mid].start <= c) lo = mid; else hi = mid;
    }
    unsigned set = ecma_ucd::scx_runs[lo].set;
    if (set == 0) return script(c) == sc;
    for (unsigned i = ecma_ucd::scx_set_offsets[set]; i < ecma_ucd::scx_set_offsets[set + 1]; ++i)
        if (ecma_ucd::scx_set_scripts[i] == sc) return true;
    return false;
}
// ECMA-262 WhiteSpace and LineTerminator: what \s matches.
inline bool is_ecma_space(char32_t c) {
    return (c >= 0x09 && c <= 0x0D) || c == 0x20 || c == 0xA0 || c == 0x1680 || (c >= 0x2000 && c <= 0x200A) ||
           c == 0x2028 || c == 0x2029 || c == 0x202F || c == 0x205F || c == 0x3000 || c == 0xFEFF;
}
inline bool is_ascii_word(char32_t c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}
inline char32_t simple_fold(char32_t c) {
    auto first = std::begin(ecma_ucd::simple_fold), last = std::end(ecma_ucd::simple_fold);
    auto it = std::lower_bound(first, last, c, [](ecma_ucd::fold_pair const& p, char32_t v) { return p.from < v; });
    return it != last && it->from == c ? it->to : c;
}
}  // namespace detail

class traits {
public:
    using char_type = char32_t;
    using size_type = std::size_t;
    using string_type = std::u32string;
    using locale_type = int;
    using char_class_type = class_mask;
    struct boost_extensions_tag {};

    static size_type length(char_type const* p) { return std::char_traits<char32_t>::length(p); }
    boost::regex_constants::syntax_type syntax_type(char_type c) const {
        return c > 0 && c < 0x7F ? boost::BOOST_REGEX_DETAIL_NS::get_default_syntax_type(static_cast<char>(c))
                                 : boost::regex_constants::syntax_char;
    }
    boost::regex_constants::escape_syntax_type escape_syntax_type(char_type c) const {
        return c > 0 && c < 0x7F ? boost::BOOST_REGEX_DETAIL_NS::get_default_escape_syntax_type(static_cast<char>(c))
                                 : boost::regex_constants::syntax_char;
    }
    char_type translate(char_type c) const { return c; }
    char_type translate_nocase(char_type c) const { return detail::simple_fold(c); }
    char_type translate(char_type c, bool icase) const { return icase ? translate_nocase(c) : c; }
    char_type tolower(char_type c) const { return detail::simple_fold(c); }
    char_type toupper(char_type c) const { return c; }
    string_type transform(char_type const* p1, char_type const* p2) const { return string_type(p1, p2); }
    string_type transform_primary(char_type const* p1, char_type const* p2) const { return string_type(p1, p2); }
    string_type lookup_collatename(char_type const* p1, char_type const* p2) const {
        return p2 - p1 == 1 ? string_type(p1, p2) : string_type();
    }
    // Names Boost asks for itself ("w", "s", "lower", "upper", "alpha"), and "#<bit>", the only
    // form the translator writes (\p{#n}, [[:#n:]]).
    char_class_type lookup_classname(char_type const* p1, char_type const* p2) const {
        std::u32string_view n(p1, static_cast<std::size_t>(p2 - p1));
        if (n == U"w") return class_mask::bit(detail::bit_w);
        if (n == U"s") return class_mask::bit(detail::bit_s);
        if (n == U"d") return class_mask::bit(detail::bit_d);
        if (n == U"lower") return class_mask::bit(detail::bit_lower);
        if (n == U"upper") return class_mask::bit(detail::bit_upper);
        if (n == U"alpha") return class_mask::bit(detail::bit_alpha);
        if (n.size() < 2 || n[0] != '#') return 0;
        class_mask flag;
        if (n.back() == 'i' || n.back() == 'n' || n.back() == 'x') {
            flag = class_mask::bit(n.back() == 'i' ? closure_any : n.back() == 'n' ? closure_not_all : closure_none);
            n.remove_suffix(1);
        }
        unsigned b = 0;
        for (char32_t c : n.substr(1)) {
            if (c < '0' || c > '9') return 0;
            b = b * 10 + (c - '0');
            if (b >= ecma_ucd::bit_count) return 0;
        }
        return class_mask::bit(b) | flag;
    }
    // Under ECMA-262's ignoreCase, a class matches c when some character of c's simple case
    // folding orbit is in it ("#ni": closure_any), and a negated class escape such as \P{Lu}
    // when some character of the orbit is outside it ("#nn": closure_not_all).
    bool isctype(char_type c, char_class_type const& m) const {
        bool const any = (m.w[closure_any / 64] >> (closure_any % 64)) & 1u;
        bool const not_all = (m.w[closure_not_all / 64] >> (closure_not_all % 64)) & 1u;
        bool const none = (m.w[closure_none / 64] >> (closure_none % 64)) & 1u;
        if (!any && !not_all && !none) return plain(c, m);
        char32_t orbit[8];
        unsigned n = 0;
        char32_t f = detail::simple_fold(c);
        orbit[n++] = c;
        if (f != c) orbit[n++] = f;
        auto first = std::begin(ecma_ucd::fold_sources), last = std::end(ecma_ucd::fold_sources);
        for (auto it = std::lower_bound(first, last, f, [](ecma_ucd::fold_pair const& p, char32_t v) { return p.from < v; });
             it != last && it->from == f && n < 8; ++it)
            if (it->to != c) orbit[n++] = it->to;
        if (none) {
            for (unsigned k = 0; k < n; ++k)
                if (plain(orbit[k], m)) return false;
            return true;
        }
        for (unsigned k = 0; k < n; ++k)
            if (plain(orbit[k], m) == any) return true;
        return false;
    }
    std::intmax_t toi(char_type const*& p1, char_type const* p2, int radix) const {
        return boost::BOOST_REGEX_DETAIL_NS::global_toi(p1, p2, radix, *this);
    }
    int value(char_type c, int radix) const {
        int v = c >= '0' && c <= '9' ? int(c - '0') : c >= 'a' && c <= 'z' ? int(c - 'a' + 10) : c >= 'A' && c <= 'Z' ? int(c - 'A' + 10) : -1;
        return v < radix ? v : -1;
    }
    locale_type imbue(locale_type l) { return l; }
    locale_type getloc() const { return 0; }
    std::string error_string(boost::regex_constants::error_type n) const {
        return boost::BOOST_REGEX_DETAIL_NS::get_default_error_string(n);
    }

    static constexpr unsigned closure_any = 510, closure_not_all = 509, closure_none = 508;

private:
    static bool plain(char32_t c, class_mask const& m) {
        int gc = -1;
        for (unsigned word = 0; word < 8; ++word) {
            std::uint64_t bits = m.w[word];
            if (word == 7) bits &= ~(std::uint64_t(7) << (closure_none % 64));
            for (; bits != 0; bits &= bits - 1) {
                unsigned b = word * 64 + static_cast<unsigned>(__builtin_ctzll(bits));
                if (test(b, c, gc)) return true;
            }
        }
        return false;
    }
    static bool test(unsigned b, char32_t c, int& gc) {
        using namespace ecma_ucd;
        if (b < bit_gc) {
            switch (b) {
            case detail::bit_w: return detail::is_ascii_word(c);
            case detail::bit_s: return detail::is_ecma_space(c);
            case detail::bit_d: return c >= '0' && c <= '9';
            case detail::bit_lower: return detail::in_binary(bin_Lowercase, c);
            case detail::bit_upper: return detail::in_binary(bin_Uppercase, c);
            case detail::bit_alpha: return detail::in_binary(bin_Alphabetic, c);
            case detail::bit_any: return c <= 0x10FFFF;
            }
            return false;
        }
        if (b < bit_bin) {
            if (gc < 0) gc = static_cast<int>(detail::general_category(c));
            return (gc_masks[b - bit_gc] >> gc) & 1u;
        }
        if (b < bit_sc) return detail::in_binary(b - bit_bin, c);
        if (b < bit_scx) return detail::script(c) == b - bit_sc;
        return detail::in_script_extensions(b - bit_scx, c);
    }
};

// ---------------------------------------------------------------------------------------------
// The translator.

enum class status : std::uint8_t {
    ok,
    syntax,       // not an ECMA-262 Pattern[+UnicodeMode]: a SyntaxError in JavaScript
    unsupported,  // valid ECMA-262 that Boost.Regex cannot run (variable-width lookbehind)
};

struct translation {
    status st = status::ok;
    std::u32string boost;  // the Boost perl-syntax pattern, over code points
    std::string why;
    // What a lookbehind holds that Boost's fixed-width backstep cannot take (for the report).
    bool lookbehind_unbounded = false;  // *, + or {n,} other than leftmost
    bool lookbehind_variable = false;   // ?, {m,n} with m < n other than leftmost
    bool lookbehind_backref = false;
};

namespace detail {

inline bool decode_utf8(std::string_view s, std::u32string& out) {
    for (std::size_t i = 0; i < s.size();) {
        auto c = static_cast<unsigned char>(s[i]);
        int n = c < 0x80 ? 1 : (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : (c & 0xF8) == 0xF0 ? 4 : 0;
        if (n == 0 || i + n > s.size()) return false;
        char32_t cp = n == 1 ? c : n == 2 ? (c & 0x1F) : n == 3 ? (c & 0x0F) : (c & 0x07);
        for (int k = 1; k < n; ++k) {
            auto cc = static_cast<unsigned char>(s[i + k]);
            if ((cc & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (cc & 0x3F);
        }
        out.push_back(cp);
        i += static_cast<std::size_t>(n);
    }
    return true;
}

inline void hex(std::u32string& out, char32_t c) {
    static constexpr char digits[] = "0123456789ABCDEF";
    out += U"\\x{";
    char buf[8];
    int n = 0;
    do {
        buf[n++] = digits[c & 0xF];
        c >>= 4;
    } while (c != 0);
    while (n > 0) out.push_back(static_cast<char32_t>(buf[--n]));
    out.push_back('}');
}

inline void number(std::u32string& out, unsigned v) {
    char buf[12];
    int n = 0;
    do {
        buf[n++] = static_cast<char>('0' + v % 10);
        v /= 10;
    } while (v != 0);
    while (n > 0) out.push_back(static_cast<char32_t>(buf[--n]));
}

inline bool is_syntax_char(char32_t c) {
    return c == '^' || c == '$' || c == '\\' || c == '.' || c == '*' || c == '+' || c == '?' || c == '(' || c == ')' ||
           c == '[' || c == ']' || c == '{' || c == '}' || c == '|';
}

struct flags {
    bool i = false, m = false, s = false;
};

// One class atom or class escape inside [...].
struct class_item {
    bool is_char = false;
    char32_t c = 0;
    // else a class: a bit of ecma::traits, positive or negated, or ranges written directly
    unsigned bit = 0;
    bool negated = false;
};

class translator {
public:
    translator(std::u32string_view p) : p_(p) {}

    translation run() {
        prescan();
        if (!t_.why.empty()) return fail_result();
        std::size_t start = out().size();
        (void)start;
        disjunction(flags{});
        if (t_.why.empty() && i_ != p_.size()) err(p_[i_] == ')' ? "unmatched )" : "unexpected character");
        if (!t_.why.empty()) return fail_result();
        for (unsigned g : backrefs_)
            if (std::find(quantified_groups_.begin(), quantified_groups_.end(), g) != quantified_groups_.end()) {
                translation r;
                r.st = status::unsupported;
                r.why = "a backreference to a group inside a quantified atom: ECMA-262 resets that group at each iteration, Boost does not";
                return r;
            }
        return std::move(t_);
    }

private:
    std::u32string_view p_;
    std::size_t i_ = 0;
    translation t_;
    unsigned groups_ = 0;               // capturing groups in the whole pattern (prescan)
    std::vector<std::u32string> names_;  // their names, by group number - 1 (empty when unnamed)
    unsigned next_group_ = 0;

    std::u32string& out() { return t_.boost; }
    translation fail_result() {
        translation r;
        r.st = t_.st == status::unsupported ? status::unsupported : status::syntax;
        r.why = t_.why;
        return r;
    }
    void err(char const* why) {
        if (t_.why.empty()) {
            t_.st = status::syntax;
            t_.why = why;
        }
    }
    bool more() const { return i_ < p_.size(); }
    char32_t peek(std::size_t k = 0) const { return i_ + k < p_.size() ? p_[i_ + k] : char32_t(0xFFFFFFFF); }
    bool eat(char32_t c) {
        if (peek() == c) {
            ++i_;
            return true;
        }
        return false;
    }

    // Counts the capturing groups and reads their names, as ECMA-262's early errors need them
    // (a \k<name> or \N may refer forward).
    void prescan() {
        bool in_class = false;
        for (std::size_t k = 0; k < p_.size(); ++k) {
            char32_t c = p_[k];
            if (c == '\\') {
                ++k;
                continue;
            }
            if (in_class) {
                if (c == ']') in_class = false;
                continue;
            }
            if (c == '[') {
                in_class = true;
                continue;
            }
            if (c != '(') continue;
            if (k + 1 < p_.size() && p_[k + 1] == '?') {
                if (k + 2 < p_.size() && p_[k + 2] == '<' && k + 3 < p_.size() && p_[k + 3] != '=' && p_[k + 3] != '!') {
                    std::size_t save = i_;
                    i_ = k + 3;
                    std::u32string name;
                    if (!group_name(name, '>')) return;
                    i_ = save;
                    if (std::find(names_.begin(), names_.end(), name) != names_.end()) {
                        err("duplicate group name");
                        return;
                    }
                    names_.push_back(name);
                    ++groups_;
                }
                continue;
            }
            names_.emplace_back();
            ++groups_;
        }
    }

    bool id_start(char32_t c) const { return c == '$' || c == '_' || in_binary(ecma_ucd::bin_ID_Start, c); }
    bool id_continue(char32_t c) const { return c == '$' || c == 0x200C || c == 0x200D || in_binary(ecma_ucd::bin_ID_Continue, c); }

    // RegExpIdentifierName, then the closing delimiter; at i_.
    bool group_name(std::u32string& name, char32_t close) {
        bool first = true;
        while (true) {
            if (!more()) {
                err("unterminated group name");
                return false;
            }
            char32_t c = p_[i_++];
            if (c == close) break;
            if (c == '\\') {
                if (!eat('u')) {
                    err("bad escape in group name");
                    return false;
                }
                if (!unicode_escape(c)) return false;
            }
            if (first ? !id_start(c) : !id_continue(c)) {
                err("bad group name character");
                return false;
            }
            name.push_back(c);
            first = false;
        }
        if (name.empty()) {
            err("empty group name");
            return false;
        }
        return true;
    }

    int hexval(char32_t c) const {
        return c >= '0' && c <= '9' ? int(c - '0') : c >= 'a' && c <= 'f' ? int(c - 'a' + 10) : c >= 'A' && c <= 'F' ? int(c - 'A' + 10) : -1;
    }
    bool hex_digits(int n, char32_t& v) {
        v = 0;
        for (int k = 0; k < n; ++k) {
            int h = hexval(peek(static_cast<std::size_t>(k)));
            if (h < 0) return false;
            v = v * 16 + static_cast<char32_t>(h);
        }
        i_ += static_cast<std::size_t>(n);
        return true;
    }
    // After "\u": u{X...} or XXXX, joining an escaped surrogate pair.
    bool unicode_escape(char32_t& c) {
        if (eat('{')) {
            c = 0;
            int n = 0;
            while (more() && peek() != '}') {
                int h = hexval(peek());
                if (h < 0) {
                    err("bad \\u{...}");
                    return false;
                }
                c = c * 16 + static_cast<char32_t>(h);
                if (c > 0x10FFFF) {
                    err("\\u{...} above 10FFFF");
                    return false;
                }
                ++i_;
                ++n;
            }
            if (n == 0 || !eat('}')) {
                err("bad \\u{...}");
                return false;
            }
            return true;
        }
        if (!hex_digits(4, c)) {
            err("bad \\uXXXX");
            return false;
        }
        if (c >= 0xD800 && c <= 0xDBFF && peek() == '\\' && peek(1) == 'u') {
            std::size_t save = i_;
            i_ += 2;
            char32_t lo;
            if (hex_digits(4, lo) && lo >= 0xDC00 && lo <= 0xDFFF) {
                c = 0x10000 + ((c - 0xD800) << 10) + (lo - 0xDC00);
            } else {
                i_ = save;
            }
        }
        return true;
    }

    // \p{...} / \P{...}, after the p or P: the bit of ecma::traits.
    bool property(unsigned& bit) {
        if (!eat('{')) {
            err("\\p without {");
            return false;
        }
        std::u32string body;
        while (more() && peek() != '}') body.push_back(p_[i_++]);
        if (!eat('}')) {
            err("unterminated \\p{");
            return false;
        }
        std::string name, value;
        auto eq = body.find('=');
        auto narrow = [](std::u32string_view s, std::string& o) {
            for (char32_t c : s) {
                if (c > 0x7F) return false;
                o.push_back(static_cast<char>(c));
            }
            return true;
        };
        if (!narrow(eq == std::u32string::npos ? std::u32string_view(body) : std::u32string_view(body).substr(0, eq), name) ||
            (eq != std::u32string::npos && !narrow(std::u32string_view(body).substr(eq + 1), value))) {
            err("bad property name");
            return false;
        }
        auto find = [](std::string_view n, ecma_ucd::name_kind k) -> int {
            for (auto const& e : ecma_ucd::property_names)
                if (e.kind == k && n == e.name) return e.value;
            return -1;
        };
        if (eq == std::u32string::npos) {
            int v = find(name, ecma_ucd::name_kind::lone_or_gc);
            if (v < 0) v = find(name, ecma_ucd::name_kind::binary);
            if (v < 0) {
                err("unknown property");
                return false;
            }
            bit = static_cast<unsigned>(v);
            return true;
        }
        if (name == "General_Category" || name == "gc") {
            int v = find(value, ecma_ucd::name_kind::lone_or_gc);
            if (v >= 0) {
                bit = static_cast<unsigned>(v);
                return true;
            }
        } else if (name == "Script" || name == "sc" || name == "Script_Extensions" || name == "scx") {
            int v = find(value, ecma_ucd::name_kind::script);
            if (v >= 0) {
                bit = static_cast<unsigned>(v) + (name == "Script" || name == "sc" ? ecma_ucd::bit_sc : ecma_ucd::bit_scx);
                return true;
            }
        }
        err("unknown property or value");
        return false;
    }

    static constexpr char32_t const* line_terminators = U"\\x{A}\\x{D}\\x{2028}\\x{2029}";

    void disjunction(flags f) {
        alternative(f);
        while (t_.why.empty() && eat('|')) {
            out().push_back('|');
            alternative(f);
        }
    }

    // lb_top: an alternative at the top of a lookbehind, whose leftmost term may be reduced.
    void alternative(flags f, bool lb_top = false) {
        left_edge_ = lb_top;
        while (t_.why.empty() && more() && peek() != '|' && peek() != ')') term(f);
        left_edge_ = false;
    }
    bool left_edge_ = false;

    void term(flags f) {
        // Only the leftmost term of a lookbehind may be reduced, and an assertion is a term:
        // (?<=^a*) and (?<=\ba?) keep their quantifiers.
        bool const left_edge = left_edge_;
        left_edge_ = false;
        char32_t c = peek();
        // Assertions: not quantifiable in u-mode.
        if (c == '^') {
            ++i_;
            out() += f.m ? std::u32string(U"(?:\\A|(?<=[") + line_terminators + U"]))" : U"\\A";
            return no_quantifier();
        }
        if (c == '$') {
            ++i_;
            out() += f.m ? std::u32string(U"(?=[") + line_terminators + U"]|\\z)" : U"\\z";
            return no_quantifier();
        }
        if (c == '\\' && (peek(1) == 'b' || peek(1) == 'B')) {
            i_ += 2;
            bool boundary = p_[i_ - 1] == 'b';
            if (!f.i) {
                out() += boundary ? U"\\b" : U"\\B";
            } else {
                // Under ignoreCase ECMA-262's word characters gain U+017F and U+212A; Boost's \b
                // reads its own word class without case folding, so it is spelled out.
                std::u32string w = U"[" + class_token(bit_w, false, f) + U"]";
                out() += boundary ? U"(?:(?<=" + w + U")(?!" + w + U")|(?<!" + w + U")(?=" + w + U"))"
                                  : U"(?:(?<=" + w + U")(?=" + w + U")|(?<!" + w + U")(?!" + w + U"))";
            }
            return no_quantifier();
        }
        if (c == '(' && peek(1) == '?' && (peek(2) == '=' || peek(2) == '!')) {
            i_ += 3;
            out() += p_[i_ - 1] == '=' ? U"(?=" : U"(?!";
            disjunction(f);
            if (!eat(')')) return err("unterminated lookahead");
            out().push_back(')');
            return no_quantifier();
        }
        if (c == '(' && peek(1) == '?' && peek(2) == '<' && (peek(3) == '=' || peek(3) == '!')) {
            i_ += 4;
            bool negative = p_[i_ - 1] == '!';
            ++lb_depth_;
            struct leave {
                unsigned& d;
                ~leave() { --d; }
            } leave_lb{lb_depth_};
            // Boost needs one fixed width per lookbehind: its top-level alternatives become
            // lookbehinds of their own, which lets alternatives of different widths through.
            std::vector<std::u32string> alts;
            std::u32string saved = std::move(out());
            out().clear();
            alternative(f, true);
            alts.push_back(std::move(out()));
            while (t_.why.empty() && eat('|')) {
                out().clear();
                alternative(f, true);
                alts.push_back(std::move(out()));
            }
            out() = std::move(saved);
            if (!t_.why.empty()) return;
            if (!eat(')')) return err("unterminated lookbehind");
            if (alts.size() == 1) {
                out() += negative ? U"(?<!" : U"(?<=";
                out() += alts[0];
                out().push_back(')');
            } else {
                if (!negative) out() += U"(?:";
                for (std::size_t k = 0; k < alts.size(); ++k) {
                    if (!negative && k) out().push_back('|');
                    out() += negative ? U"(?<!" : U"(?<=";
                    out() += alts[k];
                    out().push_back(')');
                }
                if (!negative) out().push_back(')');
            }
            return no_quantifier();
        }
        std::size_t const atom_start = out().size();
        unsigned const groups_before = next_group_;
        if (!atom(f)) return;
        if (left_edge && next_group_ == groups_before) {
            // A lookbehind is anchored only at its right end, so its leftmost quantified atom
            // needs only its minimum count: (?<=a+b) holds exactly where (?<=ab) does, and
            // (?<=a*b) where (?<=b). Boost then sees a fixed width. A capture inside stops this.
            unsigned lo = 0, hi = 0;
            bool bounded = true;
            char32_t c = peek();
            if (c == '*' || c == '?') {
                ++i_;
                lo = 0;
            } else if (c == '+') {
                ++i_;
                lo = 1;
            } else if (c == '{' && braces(lo, hi, bounded)) {
                if (bounded && hi < lo) return err("numbers out of order in {} quantifier");
            } else {
                return;
            }
            eat('?');
            c = peek();
            if (c == '*' || c == '+' || c == '?') return err("nothing to repeat");
            if (lo == 0) {
                out().resize(atom_start);
                left_edge_ = true;
            } else if (lo > 1) {
                out().push_back('{');
                number(out(), lo);
                out().push_back('}');
            }
            return;
        }
        std::size_t const before = out().size();
        quantifier();
        // ECMA-262 resets the captures inside a quantified atom at each iteration (RepeatMatcher);
        // Boost keeps the last iteration's. Only a backreference can see the difference.
        if (out().size() != before)
            for (unsigned g = groups_before; g < next_group_; ++g) quantified_groups_.push_back(g + 1);
    }

    void no_quantifier() {
        char32_t c = peek();
        if (c == '*' || c == '+' || c == '?') return err("nothing to repeat");
        if (c == '{') {
            std::size_t save = i_;
            unsigned lo, hi;
            bool bounded;
            if (braces(lo, hi, bounded)) return err("nothing to repeat");
            i_ = save;
            return err("lone {");
        }
    }

    // {n}, {n,}, {n,m} at i_; false (i_ unchanged) when it is not one.
    bool braces(unsigned& lo, unsigned& hi, bool& bounded) {
        std::size_t save = i_;
        if (!eat('{')) return false;
        auto digits = [&](unsigned& v) {
            if (!(peek() >= '0' && peek() <= '9')) return false;
            std::uint64_t x = 0;
            while (peek() >= '0' && peek() <= '9') {
                x = x * 10 + (p_[i_++] - '0');
                if (x > 0xFFFFFFFFu) x = 0xFFFFFFFFu;
            }
            v = static_cast<unsigned>(x);
            return true;
        };
        if (!digits(lo)) {
            i_ = save;
            return false;
        }
        hi = lo;
        bounded = true;
        if (eat(',')) {
            if (peek() == '}') {
                bounded = false;
            } else if (!digits(hi)) {
                i_ = save;
                return false;
            }
        }
        if (!eat('}')) {
            i_ = save;
            return false;
        }
        return true;
    }

    void quantifier() {
        char32_t c = peek();
        if (lb_depth_ > 0 && (c == '*' || c == '+')) t_.lookbehind_unbounded = true;
        if (lb_depth_ > 0 && c == '?') t_.lookbehind_variable = true;
        if (c == '*' || c == '+' || c == '?') {
            ++i_;
            out().push_back(c);
        } else if (c == '{') {
            unsigned lo, hi;
            bool bounded;
            if (!braces(lo, hi, bounded)) return err("lone {");
            if (bounded && hi < lo) return err("numbers out of order in {} quantifier");
            if (lb_depth_ > 0 && !bounded) t_.lookbehind_unbounded = true;
            if (lb_depth_ > 0 && bounded && hi != lo) t_.lookbehind_variable = true;
            out().push_back('{');
            number(out(), lo);
            if (!bounded) {
                out().push_back(',');
            } else if (hi != lo) {
                out().push_back(',');
                number(out(), hi);
            }
            out().push_back('}');
        } else {
            return;
        }
        if (eat('?')) out().push_back('?');
        c = peek();
        if (c == '*' || c == '+' || c == '?' || (c == '{' && [&] {
                std::size_t s = i_;
                unsigned a, b;
                bool bd;
                bool r = braces(a, b, bd);
                i_ = s;
                return r;
            }()))
            err("nothing to repeat");
    }

    bool atom(flags f) {
        char32_t c = p_[i_];
        switch (c) {
        case '.':
            ++i_;
            out() += f.s ? std::u32string(U"[\\x{0}-\\x{10FFFF}]") : std::u32string(U"[^") + line_terminators + U"]";
            return true;
        case '(':
            return group(f);
        case '[':
            return char_class(f);
        case '\\':
            ++i_;
            return atom_escape(f);
        case '*':
        case '+':
        case '?':
            err("nothing to repeat");
            return false;
        case '{':
            err("lone {");
            return false;
        case '}':
            err("lone }");
            return false;
        case ']':
            err("lone ]");
            return false;
        default:
            ++i_;
            literal(c, f);
            return true;
        }
    }

    void literal(char32_t c, flags) { hex(out(), c); }

    bool group(flags f) {
        ++i_;  // (
        if (!eat('?')) {
            ++next_group_;
            out().push_back('(');
            disjunction(f);
            if (!t_.why.empty()) return false;
            if (!eat(')')) return err("unterminated group"), false;
            out().push_back(')');
            return true;
        }
        if (eat(':')) {
            out() += U"(?:";
            disjunction(f);
            if (!t_.why.empty()) return false;
            if (!eat(')')) return err("unterminated group"), false;
            out().push_back(')');
            return true;
        }
        if (eat('<')) {
            std::u32string name;
            if (!group_name(name, '>')) return false;
            ++next_group_;
            out().push_back('(');
            disjunction(f);
            if (!t_.why.empty()) return false;
            if (!eat(')')) return err("unterminated group"), false;
            out().push_back(')');
            return true;
        }
        // ES2025 modifiers: (?ims-ims:...)
        flags g = f;
        bool seen[3] = {false, false, false};
        bool any = false, minus = false;
        while (more() && peek() != ':') {
            char32_t m = p_[i_++];
            if (m == '-') {
                if (minus) return err("bad modifiers"), false;
                minus = true;
                continue;
            }
            int k = m == 'i' ? 0 : m == 'm' ? 1 : m == 's' ? 2 : -1;
            if (k < 0 || seen[k]) return err("invalid group"), false;
            seen[k] = true;
            any = true;
            (k == 0 ? g.i : k == 1 ? g.m : g.s) = !minus;
        }
        if (!any || !eat(':')) return err("invalid group"), false;
        out() += g.i == f.i ? U"(?:" : g.i ? U"(?i:" : U"(?-i:";
        disjunction(g);
        if (!t_.why.empty()) return false;
        if (!eat(')')) return err("unterminated group"), false;
        out().push_back(')');
        return true;
    }

    unsigned lb_depth_ = 0;
    std::vector<unsigned> backrefs_, quantified_groups_;
    void backref(unsigned n) {
        backrefs_.push_back(n);
        if (lb_depth_ > 0) t_.lookbehind_backref = true;
        // ECMA-262: a reference to a group that has not participated matches the empty string;
        // Boost's \N fails there. (?(N)\N) is Boost's "if group N matched".
        out() += U"(?(";
        number(out(), n);
        out() += U")\\";
        number(out(), n);
        out() += U")";
    }

    bool character_escape(char32_t e, char32_t& c, bool in_class) {
        switch (e) {
        case 'f': c = 0x0C; return true;
        case 'n': c = 0x0A; return true;
        case 'r': c = 0x0D; return true;
        case 't': c = 0x09; return true;
        case 'v': c = 0x0B; return true;
        case 'c': {
            char32_t l = peek();
            if ((l >= 'a' && l <= 'z') || (l >= 'A' && l <= 'Z')) {
                ++i_;
                c = l % 32;
                return true;
            }
            err("\\c without a letter");
            return false;
        }
        case '0':
            if (peek() >= '0' && peek() <= '9') {
                err("octal escape");
                return false;
            }
            c = 0;
            return true;
        case 'x':
            if (!hex_digits(2, c)) {
                err("bad \\x");
                return false;
            }
            return true;
        case 'u':
            return unicode_escape(c);
        case '-':
            if (in_class) {
                c = '-';
                return true;
            }
            break;
        default:
            if (is_syntax_char(e) || e == '/') {
                c = e;
                return true;
            }
        }
        err("escape not valid in u-mode");
        return false;
    }

    bool atom_escape(flags f) {
        if (!more()) return err("trailing backslash"), false;
        char32_t e = p_[i_++];
        switch (e) {
        case 'd':
        case 'D':
        case 'w':
        case 'W':
        case 's':
        case 'S':
        case 'p':
        case 'P': {
            unsigned bit = e == 'd' || e == 'D' ? bit_d : e == 'w' || e == 'W' ? bit_w : bit_s;
            if ((e == 'p' || e == 'P') && !property(bit)) return false;
            bool negated = e == 'D' || e == 'W' || e == 'S' || e == 'P';
            out() += U"[";
            out() += class_token(bit, negated, f);
            out() += U"]";
            return true;
        }
        case 'k': {
            if (!eat('<')) return err("\\k without a group name"), false;
            std::u32string name;
            if (!group_name(name, '>')) return false;
            auto it = std::find(names_.begin(), names_.end(), name);
            if (it == names_.end()) return err("\\k to an unknown group"), false;
            backref(static_cast<unsigned>(it - names_.begin()) + 1);
            return true;
        }
        default:
            break;
        }
        if (e >= '1' && e <= '9') {
            std::uint64_t n = e - '0';
            while (peek() >= '0' && peek() <= '9') {
                n = n * 10 + (p_[i_++] - '0');
                if (n > 1000000) n = 1000000;
            }
            if (n > groups_) return err("backreference to a group that does not exist"), false;
            backref(static_cast<unsigned>(n));
            return true;
        }
        char32_t c;
        if (!character_escape(e, c, false)) return false;
        literal(c, f);
        return true;
    }

    // One ClassAtom; false on error.
    bool class_atom(class_item& it) {
        char32_t c = p_[i_++];
        if (c != '\\') {
            it.is_char = true;
            it.c = c;
            return true;
        }
        if (!more()) return err("trailing backslash"), false;
        char32_t e = p_[i_++];
        switch (e) {
        case 'b': it.is_char = true; it.c = 0x08; return true;
        case 'd': it.bit = bit_d; it.negated = false; return true;
        case 'D': it.bit = bit_d; it.negated = true; return true;
        case 'w': it.bit = bit_w; it.negated = false; return true;
        case 'W': it.bit = bit_w; it.negated = true; return true;
        case 's': it.bit = bit_s; it.negated = false; return true;
        case 'S': it.bit = bit_s; it.negated = true; return true;
        case 'p':
        case 'P':
            if (!property(it.bit)) return false;
            it.negated = e == 'P';
            return true;
        default:
            break;
        }
        if (e >= '1' && e <= '9') return err("backreference in a class"), false;
        if (e == 'k') return err("\\k in a class"), false;
        if (e == 'B') return err("\\B in a class"), false;
        it.is_char = true;
        return character_escape(e, it.c, true);
    }

    // A class of ecma::traits as a Boost set item: [:#n:] or [:^#n:], and under ignoreCase the
    // closures [:#ni:] and [:#nn:] (see traits::isctype).
    static std::u32string class_token(unsigned bit, bool negated, flags f) {
        std::u32string s = negated && !f.i ? U"[:^#" : U"[:#";
        number(s, bit);
        // \W, \D, \S complement ECMA-262's (case-closed) WordCharacters, digits and spaces:
        // no character of the orbit in the class ('x'); \P{...} complements the property
        // itself, then matches by case folding: some character of the orbit outside it ('n').
        if (f.i) s.push_back(!negated ? 'i' : bit <= bit_d ? 'x' : 'n');
        s += U":]";
        return s;
    }

    bool char_class(flags f) {
        ++i_;  // [
        bool negated = eat('^');
        std::vector<std::pair<char32_t, char32_t>> ranges;
        std::vector<unsigned> pos;
        std::vector<unsigned> neg;
        while (true) {
            if (!more()) return err("unterminated class"), false;
            if (peek() == ']') {
                ++i_;
                break;
            }
            class_item a;
            if (!class_atom(a)) return false;
            if (peek() == '-' && peek(1) != ']' && peek(1) != char32_t(0xFFFFFFFF)) {
                ++i_;
                class_item b;
                if (!class_atom(b)) return false;
                if (!a.is_char || !b.is_char) return err("class escape in a range"), false;
                if (a.c > b.c) return err("range out of order in a class"), false;
                ranges.emplace_back(a.c, b.c);
                continue;
            }
            if (a.is_char) ranges.emplace_back(a.c, a.c);
            else (a.negated ? neg : pos).push_back(a.bit);
        }
        // The positive classes and the ranges make one Boost set. A negated class escape stays
        // apart when there are others like it: Boost ORs the negated classes of a set into one
        // mask and asks "in none of them", where ECMA-262 wants "outside any one of them". Under
        // ignoreCase the closures carry a flag of their own, so each [:#nn:] stays apart too.
        std::u32string body;
        for (auto [lo, hi] : ranges) {
            hex(body, lo);
            if (hi != lo) {
                body.push_back('-');
                hex(body, hi);
            }
        }
        for (unsigned b : pos) body += class_token(b, false, f);
        std::vector<std::u32string> parts;
        bool join = neg.size() == 1 && !body.empty() && (!f.i || pos.empty());
        if (join) body += class_token(neg[0], true, f);
        if (!body.empty()) parts.push_back(U"[" + body + U"]");
        if (!join)
            for (unsigned b : neg) parts.push_back(U"[" + class_token(b, true, f) + U"]");
        std::u32string any_class;
        if (parts.empty()) any_class = U"(?!)";
        else if (parts.size() == 1) any_class = parts[0];
        else {
            any_class = U"(?:";
            for (std::size_t k = 0; k < parts.size(); ++k) {
                if (k) any_class.push_back('|');
                any_class += parts[k];
            }
            any_class.push_back(')');
        }
        if (!negated) {
            out() += any_class;
        } else if (parts.empty()) {
            out() += U"[\\x{0}-\\x{10FFFF}]";
        } else if (parts.size() == 1 && (neg.empty() || !f.i)) {
            out() += U"[^" + body + U"]";
        } else {
            out() += U"(?:(?!" + any_class + U")[\\x{0}-\\x{10FFFF}])";
        }
        return true;
    }

    static constexpr unsigned bit_w = 0, bit_s = 1, bit_d = 2;
};

}  // namespace detail

// Parses an ECMA-262 Pattern[+UnicodeMode] given as UTF-8 and writes its Boost form.
inline translation translate(std::string_view pattern) {
    std::u32string p;
    if (!detail::decode_utf8(pattern, p)) {
        translation t;
        t.st = status::syntax;
        t.why = "pattern is not UTF-8";
        return t;
    }
    return detail::translator(p).run();
}

// format: regex. ECMA-262 validity is a question of syntax, which the translator answers alone.
inline bool is_regex(std::string_view pattern) { return translate(pattern).st != status::syntax; }

// ---------------------------------------------------------------------------------------------
// Matching.

struct budget {
    std::uint64_t left;
    bool exhausted = false;
};

// A bidirectional iterator over UTF-8 yielding code points. Every operation spends one step of
// the search's budget; once it is spent, every iterator of the search compares equal to every
// other, so each state of Boost's matcher finds itself at the end of the input and fails.
class u8_iterator {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = char32_t;
    using difference_type = std::ptrdiff_t;
    using pointer = char32_t const*;
    using reference = char32_t;

    u8_iterator() = default;
    u8_iterator(char const* p, budget* b) : p_(p), b_(b) {}

    char32_t operator*() const {
        spend();
        auto c = static_cast<unsigned char>(p_[0]);
        if (c < 0x80) return c;
        if ((c & 0xE0) == 0xC0) return (char32_t(c & 0x1F) << 6) | (p_[1] & 0x3F);
        if ((c & 0xF0) == 0xE0) return (char32_t(c & 0x0F) << 12) | (char32_t(p_[1] & 0x3F) << 6) | (p_[2] & 0x3F);
        return (char32_t(c & 0x07) << 18) | (char32_t(p_[1] & 0x3F) << 12) | (char32_t(p_[2] & 0x3F) << 6) | (p_[3] & 0x3F);
    }
    u8_iterator& operator++() {
        spend();
        auto c = static_cast<unsigned char>(*p_);
        p_ += c < 0x80 ? 1 : (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : 4;
        return *this;
    }
    u8_iterator operator++(int) {
        u8_iterator t = *this;
        ++*this;
        return t;
    }
    u8_iterator& operator--() {
        spend();
        do --p_;
        while ((static_cast<unsigned char>(*p_) & 0xC0) == 0x80);
        return *this;
    }
    u8_iterator operator--(int) {
        u8_iterator t = *this;
        --*this;
        return t;
    }
    friend bool operator==(u8_iterator const& a, u8_iterator const& b) {
        if (a.b_ != nullptr) {
            a.spend();
            if (a.b_->exhausted) return true;
        }
        return a.p_ == b.p_;
    }
    char const* base() const { return p_; }

private:
    void spend() const {
        if (b_ != nullptr && !b_->exhausted && b_->left-- == 0) b_->exhausted = true;
    }
    char const* p_ = nullptr;
    budget* b_ = nullptr;
};

using boost_regex = boost::basic_regex<char32_t, traits>;

enum class match_result : std::uint8_t { found, not_found, limit };

struct compiled {
    status st = status::ok;
    std::string why;
    boost_regex re;
};

inline compiled compile(std::string_view pattern) {
    compiled c;
    translation t = translate(pattern);
    if (t.st != status::ok) {
        c.st = t.st;
        c.why = t.why;
        return c;
    }
    namespace rc = boost::regex_constants;
    c.re.assign(t.boost.data(), t.boost.data() + t.boost.size(), rc::perl | rc::no_except | rc::no_mod_m | rc::no_mod_s);
    if (c.re.status() != 0) {
        c.st = status::unsupported;
        c.why = std::string("Boost refuses the translation: ") +
                boost::BOOST_REGEX_DETAIL_NS::get_default_error_string(static_cast<rc::error_type>(c.re.status()));
        c.why += t.lookbehind_unbounded ? " [lookbehind: unbounded]"
                 : t.lookbehind_backref  ? " [lookbehind: backreference]"
                 : t.lookbehind_variable ? " [lookbehind: bounded variable width]"
                                         : " [lookbehind: alternatives of different widths inside a group]";
    }
    return c;
}

// Steps of the iterator a search may spend: a base, plus some per byte so that a linear match
// on a long instance (about 6 steps per character for ^[a-z]+$) never reaches it.
inline constexpr std::uint64_t default_budget = 10'000'000;
inline constexpr std::uint64_t budget_per_byte = 64;

template <class Allocator = std::allocator<boost::sub_match<u8_iterator>>>
match_result search(boost_regex const& re, std::string_view text, std::uint64_t steps = default_budget,
                    Allocator const& a = Allocator()) {
    budget b{steps + budget_per_byte * text.size()};
    u8_iterator first(text.data(), &b), last(text.data() + text.size(), &b);
    boost::match_results<u8_iterator, Allocator> m(a);
    bool found = boost::regex_search(first, last, m, re, boost::match_default);
    if (b.exhausted) return match_result::limit;
    return found ? match_result::found : match_result::not_found;
}

}  // namespace ecma
