#include "common.hpp"
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/uritemplate.h>
int main() {
    return run([](const std::string& f, const std::string& s) -> char {
        using namespace sourcemeta::core;
        if (f == "iri") return URI::is_iri(s) ? '1' : '0';
        if (f == "iri-reference") return URI::is_iri_reference(s) ? '1' : '0';
        if (f == "uri-template") return URITemplate::is_uritemplate(s) ? '1' : '0';
        return 'N';
    });
}
