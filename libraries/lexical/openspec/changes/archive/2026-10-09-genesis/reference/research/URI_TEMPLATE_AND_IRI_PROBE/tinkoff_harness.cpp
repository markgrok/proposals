#include "common.hpp"
#include <uri-template/uri-template.h>
int main() {
    return run([](const std::string& f, const std::string& s) -> char {
        if (f != "uri-template") return 'N';
        try { URI::Template::ParseTemplate(s); return '1'; } catch (const std::exception&) { return '0'; }
    });
}
