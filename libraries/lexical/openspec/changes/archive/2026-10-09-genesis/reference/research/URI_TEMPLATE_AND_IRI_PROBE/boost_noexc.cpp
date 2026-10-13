#include <boost/assert/source_location.hpp>
#include <cstdlib>
#include <exception>
namespace boost {
[[noreturn]] void throw_exception(std::exception const&) { std::abort(); }
[[noreturn]] void throw_exception(std::exception const&, boost::source_location const&) { std::abort(); }
}
