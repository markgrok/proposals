#include <boost/json/basic_parser_impl.hpp>
#include <cstdio>
#include <string>
#include <string_view>
// A SAX handler that prints events: keys in source order, numbers as their text.
struct handler {
    constexpr static std::size_t max_object_size = std::size_t(-1);
    constexpr static std::size_t max_array_size = std::size_t(-1);
    constexpr static std::size_t max_key_size = std::size_t(-1);
    constexpr static std::size_t max_string_size = std::size_t(-1);
    std::string out, part;
    bool on_document_begin(boost::system::error_code&) { return true; }
    bool on_document_end(boost::system::error_code&) { return true; }
    bool on_object_begin(boost::system::error_code&) { out += "{"; return true; }
    bool on_object_end(std::size_t, boost::system::error_code&) { out += "}"; return true; }
    bool on_array_begin(boost::system::error_code&) { out += "["; return true; }
    bool on_array_end(std::size_t, boost::system::error_code&) { out += "]"; return true; }
    bool on_key_part(std::string_view s, std::size_t, boost::system::error_code&) { part += s; return true; }
    bool on_key(std::string_view s, std::size_t, boost::system::error_code&) { out += "key:" + part + std::string(s) + " "; part.clear(); return true; }
    bool on_string_part(std::string_view s, std::size_t, boost::system::error_code&) { part += s; return true; }
    bool on_string(std::string_view s, std::size_t, boost::system::error_code&) { out += "str:" + part + std::string(s) + " "; part.clear(); return true; }
    bool on_number_part(std::string_view s, boost::system::error_code&) { part += s; return true; }
    bool on_int64(std::int64_t, std::string_view s, boost::system::error_code&) { out += "int:" + part + std::string(s) + " "; part.clear(); return true; }
    bool on_uint64(std::uint64_t, std::string_view s, boost::system::error_code&) { out += "uint:" + part + std::string(s) + " "; part.clear(); return true; }
    bool on_double(double, std::string_view s, boost::system::error_code&) { out += "num-text:" + part + std::string(s) + " "; part.clear(); return true; }
    bool on_bool(bool b, boost::system::error_code&) { out += b ? "true " : "false "; return true; }
    bool on_null(boost::system::error_code&) { out += "null "; return true; }
    bool on_comment_part(std::string_view, boost::system::error_code&) { return true; }
    bool on_comment(std::string_view, boost::system::error_code&) { return true; }
};
int main() {
    boost::json::parse_options opt;
    opt.numbers = boost::json::number_precision::none;  // no double conversion at all
    boost::json::basic_parser<handler> p(opt);
    std::string_view text = R"({"b":1,"2":2,"a":0.1000000000000000000000000000000001,"1":[18446744073709551616,-9223372036854775808,"xé"],"a":true})";
    boost::system::error_code ec;
    p.write_some(false, text.data(), text.size(), ec);
    std::printf("%s\n%s\n", ec ? ec.message().c_str() : "ok", p.handler().out.c_str());
    std::string_view bad = R"({"a":1,)";
    boost::json::basic_parser<handler> q(opt);
    q.write_some(false, bad.data(), bad.size(), ec);
    std::printf("malformed -> %s\n", ec.message().c_str());
}
