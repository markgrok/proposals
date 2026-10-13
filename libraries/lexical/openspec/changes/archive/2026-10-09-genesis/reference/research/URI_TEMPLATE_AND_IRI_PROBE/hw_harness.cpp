#include "common.hpp"
#include "handwritten.hpp"
int main() {
    return run([](const std::string& f, const std::string& s) -> char {
        if (f == "iri") return hw::iri(s, true) ? '1' : '0';
        if (f == "iri-reference") return hw::iri(s, false) ? '1' : '0';
        if (f == "uri-template") return hw::uri_template(s) ? '1' : '0';
        return 'N';
    });
}
