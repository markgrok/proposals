#include "common.hpp"
#include <skyr/url.hpp>
int main() {
    return run([](const std::string& f, const std::string& s) -> char {
        if (f == "iri") return skyr::make_url(s) ? '1' : '0';
        return 'N';
    });
}
