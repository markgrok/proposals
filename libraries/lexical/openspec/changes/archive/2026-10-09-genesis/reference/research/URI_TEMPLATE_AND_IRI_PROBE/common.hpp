// Reads "format \t hex(string)" lines from stdin; writes 1 (accepted), 0 (rejected) or N (no parser).
#include <iostream>
#include <string>
template <class Judge> int run(Judge judge) {
    std::string line;
    while (std::getline(std::cin, line)) {
        auto tab = line.find('\t');
        std::string format = line.substr(0, tab), hex = line.substr(tab + 1), s;
        for (std::size_t i = 0; i + 1 < hex.size(); i += 2) s.push_back(static_cast<char>(std::stoi(hex.substr(i, 2), nullptr, 16)));
        std::cout << judge(format, s) << '\n';
    }
    return 0;
}
