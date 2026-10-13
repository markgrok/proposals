// A minimal Boost.Locale program: a generator, translate (with and without a catalog), format,
// case conversion and a charset conversion.
#include <boost/locale.hpp>

#include <ctime>
#include <iostream>

#if defined(__cpp_exceptions)
#define STEP(label, expr)                                                          \
    try {                                                                          \
        std::cout << label << ": " << expr << "\n";                               \
    } catch (std::exception const& e) {                                            \
        std::cout << label << ": threw " << e.what() << "\n";                     \
    }
#else
#define STEP(label, expr) std::cout << label << ": " << expr << "\n";
#endif

int main() {
    std::cout << "backends:";
    for (auto const& b : boost::locale::localization_backend_manager::global().get_all_backends()) std::cout << ' ' << b;
    std::cout << "\n";
    boost::locale::generator gen;
    gen.add_messages_path("locale");
    gen.add_messages_domain("probe");
    std::locale loc;
#if defined(__cpp_exceptions)
    try {
        loc = gen("pt_BR.UTF-8");
    } catch (std::exception const& e) {
        std::cout << "generator threw " << e.what() << "\n";
        return 1;
    }
#else
    loc = gen("pt_BR.UTF-8");
#endif
    std::locale::global(loc);
    std::cout.imbue(loc);
    std::cout << "name=" << std::use_facet<boost::locale::info>(loc).name() << " utf8=" << std::use_facet<boost::locale::info>(loc).utf8() << "\n";
    STEP("translate", boost::locale::translate("Hello World"));
    STEP("plural", boost::locale::format(boost::locale::translate("{1} file", "{1} files", 3)) % 3);
    STEP("format", boost::locale::format("{1} has {2} items costing {3,num}") % "Ann" % 3 % 1234567.5);
    STEP("upper", boost::locale::to_upper("\xc3\xa9" "cole"));
    STEP("conv latin1", boost::locale::conv::to_utf<char>("caf\xe9", "ISO-8859-1"));
    STEP("conv cp1252", boost::locale::conv::to_utf<char>("caf\xe9", "windows-1252"));
    STEP("conv shift_jis", boost::locale::conv::to_utf<char>("\x82\xa0", "Shift_JIS"));
    std::cout << "date: " << boost::locale::as::date << std::time_t(86400 * 365) << boost::locale::as::posix << "\n";
    return 0;
}
