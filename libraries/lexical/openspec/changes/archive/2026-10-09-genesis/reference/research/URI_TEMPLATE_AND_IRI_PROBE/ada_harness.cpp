#include "common.hpp"
#include <ada.h>
int main() {
    return run([](const std::string& f, const std::string& s) -> char {
        if (f == "iri") return ada::can_parse(s) ? '1' : '0';
        return 'N';
    });
}
