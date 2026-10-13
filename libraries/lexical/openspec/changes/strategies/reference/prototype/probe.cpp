// Probes of Boost.Regex 1.92 behaviour that the prototype depends on, one case per run:
//   probe <case> [n]
// Built with and without exceptions; under BOOST_NO_EXCEPTIONS boost::throw_exception prints and
// aborts, as webcpp's tools/throw_exception.cpp does.
#include "ecma_regex.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

#if defined(PROBE_ADL)
// An overload of boost::re_detail_500::raise_error for ecma::traits, found by argument-dependent
// lookup from perl_matcher's unqualified call (perl_matcher_non_recursive.hpp:206, 216, 247):
// a non-template beats Boost's template. It returns instead of throwing.
namespace ecma {
inline int adl_raised = 0;
inline void raise_error(boost::regex_traits_wrapper<ecma::traits> const&, boost::regex_constants::error_type code) {
    ++adl_raised;
    if (adl_raised == 1) std::fprintf(stderr, "intercepted raise_error(%d)\n", int(code));
}
}  // namespace ecma
#endif

#if defined(BOOST_NO_EXCEPTIONS)
namespace boost {
void throw_exception(std::exception const& e) {
    std::fprintf(stderr, "throw_exception: %s\n", e.what());
    std::abort();
}
void throw_exception(std::exception const& e, boost::source_location const&) { throw_exception(e); }
}  // namespace boost
#endif

namespace {

double now() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

char const* name(ecma::match_result r) {
    return r == ecma::match_result::found ? "found" : r == ecma::match_result::not_found ? "not_found" : "limit";
}

// Through the prototype: compile, then search with a budget.
int ecma_case(char const* pattern, std::string const& subject, std::uint64_t budget) {
    ecma::compiled c = ecma::compile(pattern);
    if (c.st != ecma::status::ok) {
        std::printf("compile: status %d (%s)\n", int(c.st), c.why.c_str());
        return 0;
    }
    double t0 = now();
    if (budget == ecma::default_budget) budget += ecma::budget_per_byte * subject.size();  // as ecma::search
    ecma::budget b{budget};
    ecma::u8_iterator first(subject.data(), &b), last(subject.data() + subject.size(), &b);
    boost::match_results<ecma::u8_iterator> m;
#if !defined(BOOST_NO_EXCEPTIONS)
    try {
#endif
        bool found = boost::regex_search(first, last, m, c.re);
        std::printf("%s, %.3f s, %llu steps spent\n", b.exhausted ? "limit" : found ? "found" : "not_found", now() - t0,
                    static_cast<unsigned long long>(budget - (b.exhausted ? 0 : b.left)));
#if !defined(BOOST_NO_EXCEPTIONS)
    } catch (std::exception const& e) {
        std::printf("Boost threw \"%s\" after %.3f s, %llu steps spent\n", e.what(), now() - t0,
                    static_cast<unsigned long long>(budget - b.left));
    }
#endif
    return 0;
}

// Boost.Regex alone on a char32_t pattern, with ecma::traits, no translation.
int raw_case(std::u32string const& pattern, std::string const& subject, boost::match_flag_type mf = boost::match_default) {
    namespace rc = boost::regex_constants;
    ecma::boost_regex re(pattern.data(), pattern.data() + pattern.size(), rc::perl | rc::no_except);
    if (re.status() != 0) {
        std::printf("does not compile: status %d (%s)\n", re.status(),
                    boost::BOOST_REGEX_DETAIL_NS::get_default_error_string(static_cast<rc::error_type>(re.status())));
        return 0;
    }
    ecma::u8_iterator first(subject.data(), nullptr), last(subject.data() + subject.size(), nullptr);
    std::printf("%s\n", boost::regex_search(first, last, re, mf) ? "match" : "no match");
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    std::string c = argv[1];
    long n = argc > 2 ? std::atol(argv[2]) : 0;
    std::uint64_t big = 1'000'000'000'000ull;
    if (c == "catastrophic1") return ecma_case("(a*)*b", std::string(40, 'a'), ecma::default_budget);
    if (c == "catastrophic2") return ecma_case("(x+x+)+y", std::string(30, 'x'), ecma::default_budget);
    if (c == "catastrophic3")
        return ecma_case("^(\\w+\\s?)*$", "An input string that takes a long time or even makes this regex to hang!",
                         ecma::default_budget);
    if (c == "catastrophic4") return ecma_case("^(?:a?){30}a{30}$", std::string(30, 'a'), ecma::default_budget);
    // The same with an unlimited budget: Boost's own limit decides.
    if (c == "nobudget1") return ecma_case("(a*)*b", std::string(40, 'a'), big);
    if (c == "nobudget2") return ecma_case("(x+x+)+y", std::string(30, 'x'), big);
    if (c == "nobudget3")
        return ecma_case("^(\\w+\\s?)*$", "An input string that takes a long time or even makes this regex to hang!", big);
    // Long subjects: the backtracking stack (BOOST_REGEX_MAX_BLOCKS blocks of BOOST_REGEX_BLOCKSIZE).
    if (c == "stack") return ecma_case("^(?:a|b)*$", std::string(static_cast<std::size_t>(n), 'a'), ecma::default_budget);
    if (c == "stack-big") return ecma_case("^(?:a|b)*$", std::string(static_cast<std::size_t>(n), 'a'), big);
    if (c == "stack-cap") return ecma_case("^(a|bc)*$", std::string(static_cast<std::size_t>(n), 'a'), big);
    if (c == "stack-cap2") return ecma_case("^(?:(a)|bc)*$", std::string(static_cast<std::size_t>(n), 'a'), big);
    if (c == "stack-lazy") return ecma_case("^(?:a|bc)*?$", std::string(static_cast<std::size_t>(n), 'a'), big);
    if (c == "stack-nested") return ecma_case("^(?:((((((((((a))))))))))|bc)*$", std::string(static_cast<std::size_t>(n), 'a'), ecma::default_budget);
    if (c == "nested-fail") return ecma_case("^(?:((((((((((((((((((((a))))))))))))))))))))|b)*c", std::string(static_cast<std::size_t>(n), 'a'), ecma::default_budget);
    if (c == "linear") return ecma_case("^[a-z]+$", std::string(static_cast<std::size_t>(n), 'a'), ecma::default_budget);
    if (c == "linear-group") return ecma_case("^(?:[a-z])+$", std::string(static_cast<std::size_t>(n), 'a'), ecma::default_budget);
    // Boost's own syntax, through ecma::traits, to show what the translator works around.
    if (c == "raw-lookbehind-var") return raw_case(U"(?<=a+)b", "aab");
    if (c == "raw-lookbehind-alts") return raw_case(U"(?<=ab|c)x", "abx");
    if (c == "raw-lookbehind-bounded") return raw_case(U"(?<=a{1,2})b", "aab");
    if (c == "raw-lookbehind-expanded") return raw_case(U"(?:(?<=a)|(?<=aa))b", "aab");
    if (c == "raw-lookbehind-group-alts") return raw_case(U"(?<=(?:ab|c)d)x", "abdx");
    if (c == "raw-lookbehind-group-rep") return raw_case(U"(?<=(?:ab){2})x", "ababx");
    if (c == "raw-dot-12028") return raw_case(U"^.$", "\xF0\x92\x80\xA8", boost::match_not_dot_newline);  // U+12028
    if (c == "raw-dot-10085") return raw_case(U"^.$", "\xF0\x90\x82\x85", boost::match_not_dot_newline);  // U+10085
    if (c == "raw-dot-0c") return raw_case(U"^.$", "\x0C", boost::match_not_dot_newline);
    if (c == "raw-backref-unset") return raw_case(U"^(?:(a)|b)\\1$", "b");
    if (c == "raw-bad-search") {
        ecma::boost_regex re;  // empty: never compiled
        std::string s = "x";
        ecma::u8_iterator first(s.data(), nullptr), last(s.data() + 1, nullptr);
        std::printf("%d\n", int(boost::regex_search(first, last, re)));
        return 0;
    }
    // ECMA-262 semantics through the translator, for the write-up.
    if (c == "sem-iteration-reset") return ecma_case("^(?:(a)|b)+\\1$", "ab", ecma::default_budget);
    if (c == "sem-forward-ref") return ecma_case("\\1(a)", "a", ecma::default_budget);
    if (c == "sem-lookbehind-capture") return ecma_case("(?<=(\\d)(\\d))x\\1", "12x1", ecma::default_budget);
    std::printf("unknown case\n");
    return 2;
}
