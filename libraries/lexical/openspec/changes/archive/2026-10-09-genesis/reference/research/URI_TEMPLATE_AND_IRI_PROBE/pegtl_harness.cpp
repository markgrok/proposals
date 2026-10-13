#include "common.hpp"
#include <tao/pegtl.hpp>
#include <tao/pegtl/example/iri.hpp>
namespace pegtl = tao::pegtl;
template <class Rule> char judge(const std::string& s) {
    try {
        pegtl::text_view_input<pegtl::scan::lf> in(s);
        return pegtl::parse<pegtl::seq<Rule, pegtl::eof>>(in) ? '1' : '0';
    } catch (const std::exception&) {
        return '0';
    }
}
int main() {
    return run([](const std::string& f, const std::string& s) -> char {
        if (f == "iri") return judge<pegtl::iri::IRI>(s);
        if (f == "iri-reference") return judge<pegtl::iri::IRI_reference>(s);
        return 'N';
    });
}
