#include <boost/charconv.hpp>
#include <cstdio>
#include <cstring>
int main() {
    double value = 0;
    char const text[] = "1234567.5";
    auto parsed = boost::charconv::from_chars(text, text + std::strlen(text), value);
    char buffer[64];
    auto written = boost::charconv::to_chars(buffer, buffer + sizeof buffer, value);
    *written.ptr = '\0';
    std::printf("%s %d\n", buffer, parsed.ec == std::errc());
    return parsed.ec == std::errc() && std::strcmp(buffer, "1234567.5") == 0 ? 0 : 1;
}
