// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 JSON text read with the exact `Number` of each number whose double denotes another: the table of
 those numbers, by JSON Pointer, and the `boost::json::basic_parser` handler that fills it while it
 builds the value.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_JSON_EXACT_HANDLER_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_JSON_EXACT_HANDLER_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/json/json_resource.hpp>
#include <webcpp/json_schema/detail/json/pointer.hpp>
#include <webcpp/json_schema/detail/numbers/decompose.hpp>
#include <webcpp/json_schema/numbers/number.hpp>

#include <boost/container/map.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/storage_ptr.hpp>
#include <boost/json/string.hpp>
#include <boost/json/string_view.hpp>
#include <boost/json/value.hpp>
#include <boost/json/value_stack.hpp>
#include <boost/system/error_code.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace webcpp::json_schema::detail {

/** A node of @ref exact_table looked up: its parent and its token, read from a pointer or a key
    without allocating. */
struct exact_table_step_view {
    std::size_t parent{};
    std::string_view token;
};

/** A node of @ref exact_table stored: its parent and its escaped token, owned so that the table
    outlives the text it was read from. */
template <class Alloc>
struct exact_table_step {
    std::size_t parent{};
    std::basic_string<char, std::char_traits<char>, Alloc> token;
};

/**
 Orders a node of @ref exact_table by its parent, then its token's bytes; transparent for a step
 and its view alike, so a lookup never allocates or copies a token.

 Tip: the order never depends on where a token would hash, so a document whose keys are chosen to
 collide costs the same to insert and to look up as one whose keys are not (the fix for the
 flooding of a prior, hashed version of this table).
*/
struct exact_table_step_less {
    using is_transparent = void;

    template <class Step>
    static exact_table_step_view view(const Step& s) noexcept {
        return {.parent = s.parent, .token = s.token};
    }

    template <class A, class B>
    bool operator()(const A& a, const B& b) const noexcept {
        const exact_table_step_view x = view(a);
        const exact_table_step_view y = view(b);
        return x.parent != y.parent ? x.parent < y.parent : x.token < y.token;
    }
};

namespace impl {
template <class Traits, class = void>
struct exact_table_compare {
    using type = exact_table_step_less;
};

template <class Traits>
struct exact_table_compare<Traits, std::void_t<typename Traits::exact_compare>> {
    using type = Traits::exact_compare;
};
}  // namespace impl

/** The order @ref exact_table keeps by default: `Traits::exact_compare` when it names one, else
    @ref exact_table_step_less. Only a test names one, to count the comparisons a parse performs;
    `exact_handler` always takes the default. */
template <class Traits>
using exact_table_compare_t = impl::exact_table_compare<Traits>::type;

/**
 The exact `Number` of each number of a document whose double denotes another, by its JSON Pointer
 from the document's root.

 A pointer is held as a path of nodes, each its parent and its token, escaped as a pointer spells
 it, so that a token is stored once however many numbers lie below it, and the memory stays
 linear in the text: a table of whole pointers would hold a long key once per number under it.

 Tip: `steps_` orders its nodes instead of hashing them, so an insertion or a lookup costs at most
 `O(log n)` comparisons, each at most the token's length, whatever a document's keys are: the
 bound untrusted JSON cannot widen by choosing them.
*/
template <class Traits, class Compare = exact_table_compare_t<Traits>>
class exact_table {
public:
    /** The allocator of the table, the parsed document's. */
    using allocator_type = Traits::allocator_type;

    /** The number type of the traits. */
    using number_type = Traits::number_type;

    /** A number's exact value, and the double the text was read as. */
    struct entry {
        /** The lexeme as a `Number`, when `representable`. */
        number_type value;
        /** Whether the `Number` holds the lexeme; false when it is out of its range. */
        bool representable;
        /** The double the value holds there: the entry counts only while it matches. */
        double read_as;
    };

    /** The node of the document's root, whose pointer is empty. */
    static constexpr std::size_t root = 0;

    /** An empty table, which allocates through a copy of `a`; `comp` orders `steps_` and is
        given only by a test that counts its comparisons. */
    explicit exact_table(const allocator_type& a, Compare comp = Compare{})
        : steps_(comp, step_allocator(a)),
          entries_(1, std::nullopt, entry_allocator(a)),
          allocator_(a) {}

    /**
     The entry of `pointer`, or nullptr: for a pointer that is not one, names no number recorded,
     or names one with a token spelled otherwise than its canonical form (an index with a leading
     zero, a `~` not followed by `0` or `1`).
    */
    [[nodiscard]] const entry* find(std::string_view pointer) const noexcept {
        if (recorded_ == 0 || (!pointer.empty() && pointer.front() != '/')) {
            return nullptr;
        }
        std::size_t node = root;
        while (!pointer.empty()) {
            pointer.remove_prefix(1);
            const std::size_t end = std::min(pointer.find('/'), pointer.size());
            std::string_view token = pointer;
            token.remove_suffix(pointer.size() - end);
            const auto child = child_of(node, token);
            if (!child) {
                return nullptr;
            }
            node = *child;
            pointer.remove_prefix(end);
        }
        if (node >= entries_.size()) {
            return nullptr;
        }
        const std::optional<entry>& slot = entries_[node];
        return slot.has_value() ? &*slot : nullptr;
    }

    /** The entry of the node `node`, or nullptr when it holds none. */
    [[nodiscard]] const entry* entry_at(std::size_t node) const noexcept {
        if (node >= entries_.size()) {
            return nullptr;
        }
        const std::optional<entry>& slot = entries_[node];
        return slot.has_value() ? &*slot : nullptr;
    }

    /** Whether no number is recorded. */
    [[nodiscard]] bool empty() const noexcept { return recorded_ == 0; }

    /** The child of `parent` whose escaped token is `token`, if there is one. */
    [[nodiscard]] std::optional<std::size_t> child_of(std::size_t parent,
                                                      std::string_view token) const noexcept {
        const auto found = steps_.find(exact_table_step_view{.parent = parent, .token = token});
        if (found == steps_.end()) {
            return std::nullopt;
        }
        return found->second;
    }

    /** The child of `parent` whose escaped token is `token`, added when there is none. */
    std::size_t add_child(std::size_t parent, std::string_view token) {
        if (const auto child = child_of(parent, token)) {
            return *child;
        }
        const std::size_t node = entries_.size();
        steps_.emplace(step{.parent = parent, .token = string(token, char_allocator(allocator_))},
                       node);
        entries_.push_back(std::nullopt);
        return node;
    }

    /** Records `e` at `node`, in place of what it held. */
    void insert(std::size_t node, const entry& e) {
        if (!entries_[node]) {
            ++recorded_;
        }
        entries_[node] = e;
    }

    /** Forgets what `node` held. */
    void erase(std::size_t node) noexcept {
        if (node < entries_.size() && entries_[node]) {
            entries_[node].reset();
            --recorded_;
        }
    }

    /** The order `steps_` keeps, with the comparisons it has counted when `Compare` counts them;
        given only for a test, never read by @ref exact_handler. */
    [[nodiscard]] Compare key_compare() const { return steps_.key_comp(); }

private:
    template <class T>
    using rebound = std::allocator_traits<allocator_type>::template rebind_alloc<T>;
    using char_allocator = rebound<char>;
    using string = std::basic_string<char, std::char_traits<char>, char_allocator>;
    using step = exact_table_step<char_allocator>;
    using step_allocator = rebound<std::pair<const step, std::size_t>>;
    using entry_allocator = rebound<std::optional<entry>>;

    boost::container::map<step, std::size_t, Compare, step_allocator> steps_;
    std::vector<std::optional<entry>, entry_allocator> entries_;
    allocator_type allocator_;
    std::size_t recorded_{};
};

/**
 A `boost::json::basic_parser` handler that builds the value through Boost.JSON's `value_stack`,
 which is iterative, and records in an @ref exact_table each number whose lexeme, read as `Number`,
 differs from what the double's shortest round-trip text reads as.

 A number that is not recorded erases what its pointer held, so that the last of duplicate keys
 wins in the table as in the value.

 Tip: the value stack's own temporaries live in the scratch resource, not in the document's
 storage, which never frees. The path of the current value is resolved to the table's nodes
 lazily, a level at a time, and remembered, so that a key is escaped and compared against the
 table's order at most twice however many numbers lie below it.
*/
template <class Traits>
class exact_handler {
public:
    /** The most members of an object, Boost.JSON's own. */
    static constexpr std::size_t max_object_size = boost::json::object::max_size();
    /** The most elements of an array, Boost.JSON's own. */
    static constexpr std::size_t max_array_size = boost::json::array::max_size();
    /** The longest key, Boost.JSON's own. */
    static constexpr std::size_t max_key_size = boost::json::string::max_size();
    /** The longest string, Boost.JSON's own. */
    static constexpr std::size_t max_string_size = boost::json::string::max_size();

    /**
     A handler that builds the value in `storage`, keeps its temporaries in `scratch` and records
     in `table`; `scratch` and `table` must outlive it.
    */
    exact_handler(boost::json::storage_ptr storage, std::pmr::memory_resource& scratch,
                  exact_table<Traits>& table)
        : scratch_(scratch),
          stack_(boost::json::storage_ptr(&scratch_)),
          path_(&scratch),
          key_(&scratch),
          lexeme_(&scratch),
          token_(&scratch),
          table_(&table) {
        stack_.reset(std::move(storage));
    }

    /** Not copyable: the value stack is not. */
    exact_handler(const exact_handler&) = delete;
    /** Not copyable: the value stack is not. */
    exact_handler& operator=(const exact_handler&) = delete;

    /** The value built, in the storage the handler was given. */
    boost::json::value release() { return stack_.release(); }

    /** Begins the document. */
    bool on_document_begin(boost::system::error_code& /*ec*/) { return true; }

    /** Ends the document. */
    bool on_document_end(boost::system::error_code& /*ec*/) { return true; }

    /** Opens an object. */
    bool on_object_begin(boost::system::error_code& /*ec*/) { return open(false); }

    /** Closes an object of `n` members. */
    bool on_object_end(std::size_t n, boost::system::error_code& /*ec*/) {
        path_.pop_back();
        stack_.push_object(n);
        return next();
    }

    /** Opens an array. */
    bool on_array_begin(boost::system::error_code& /*ec*/) { return open(true); }

    /** Closes an array of `n` elements. */
    bool on_array_end(std::size_t n, boost::system::error_code& /*ec*/) {
        path_.pop_back();
        stack_.push_array(n);
        return next();
    }

    /** A part of a key, more to come. */
    bool on_key_part(boost::json::string_view s, std::size_t /*n*/,
                     boost::system::error_code& /*ec*/) {
        stack_.push_chars(s);
        key_.append(s.data(), s.size());
        return true;
    }

    /** The last part of a key. */
    bool on_key(boost::json::string_view s, std::size_t /*n*/, boost::system::error_code& /*ec*/) {
        stack_.push_key(s);
        key_.append(s.data(), s.size());
        level& member = path_.back();
        member.key.swap(key_);
        member.node = unknown;
        key_.clear();
        return true;
    }

    /** A part of a string, more to come. */
    bool on_string_part(boost::json::string_view s, std::size_t /*n*/,
                        boost::system::error_code& /*ec*/) {
        stack_.push_chars(s);
        return true;
    }

    /** The last part of a string. */
    bool on_string(boost::json::string_view s, std::size_t /*n*/,
                   boost::system::error_code& /*ec*/) {
        stack_.push_string(s);
        return next();
    }

    /** A part of a number's lexeme, more to come. */
    bool on_number_part(boost::json::string_view s, boost::system::error_code& /*ec*/) {
        lexeme_.append(s.data(), s.size());
        return true;
    }

    /** An integer that `std::int64_t` holds. */
    bool on_int64(std::int64_t i, boost::json::string_view /*s*/,
                  boost::system::error_code& /*ec*/) {
        stack_.push_int64(i);
        lexeme_.clear();
        forget();
        return next();
    }

    /** An integer that only `std::uint64_t` holds. */
    bool on_uint64(std::uint64_t u, boost::json::string_view /*s*/,
                   boost::system::error_code& /*ec*/) {
        stack_.push_uint64(u);
        lexeme_.clear();
        forget();
        return next();
    }

    /** Any other number, whose lexeme ends with `s`. */
    bool on_double(double d, boost::json::string_view s, boost::system::error_code& /*ec*/) {
        lexeme_.append(s.data(), s.size());
        record(d);
        stack_.push_double(d);
        lexeme_.clear();
        return next();
    }

    /** A boolean. */
    bool on_bool(bool b, boost::system::error_code& /*ec*/) {
        stack_.push_bool(b);
        return next();
    }

    /** A null. */
    bool on_null(boost::system::error_code& /*ec*/) {
        stack_.push_null();
        return next();
    }

    /** A part of a comment, ignored. */
    bool on_comment_part(boost::json::string_view /*s*/, boost::system::error_code& /*ec*/) {
        return true;
    }

    /** The last part of a comment, ignored. */
    bool on_comment(boost::json::string_view /*s*/, boost::system::error_code& /*ec*/) {
        return true;
    }

private:
    /** A level's node not looked up since its current value began. */
    static constexpr std::size_t unknown = std::numeric_limits<std::size_t>::max();
    /** A level's node looked up and not in the table. */
    static constexpr std::size_t absent = unknown - 1;

    /** An open array or object, at its current value. */
    struct level {
        bool array{};
        std::uint64_t index{};
        std::pmr::string key;
        std::size_t node{};
    };

    bool open(bool array) {
        path_.push_back({
            .array = array,
            .index = 0,
            .key = std::pmr::string(path_.get_allocator()),
            .node = unknown,
        });
        return true;
    }

    bool next() {
        if (!path_.empty() && path_.back().array) {
            ++path_.back().index;
            path_.back().node = unknown;
        }
        return true;
    }

    /**
     The table's node of the current value: looked up, or added when `add`; `absent` when not
     added and some level is not in the table.
    */
    std::size_t node_of_current(bool add) {
        std::size_t parent = exact_table<Traits>::root;
        for (level& l : path_) {
            if (l.node == unknown || (add && l.node == absent)) {
                token_.clear();
                if (l.array) {
                    append_index(token_, l.index);
                } else {
                    append_token(token_, l.key);
                }
                const std::string_view token = std::string_view(token_).substr(1);
                if (add) {
                    l.node = table_->add_child(parent, token);
                } else {
                    l.node = table_->child_of(parent, token).value_or(absent);
                }
            }
            if (l.node == absent) {
                return absent;
            }
            parent = l.node;
        }
        return parent;
    }

    /** Erases what the current value's pointer held, when the table holds anything. */
    void forget() {
        if (table_->empty()) {
            return;
        }
        const std::size_t node = node_of_current(false);
        if (node != absent) {
            table_->erase(node);
        }
    }

    /**
     Records the lexeme at the current value's pointer unless the double `d` denotes the same
     `Number`: its shortest round-trip text, never its fixed notation, which spells the binary
     value's digits (2^60 as 1152921504606846976).
    */
    void record(double d) {
        using number_type = Traits::number_type;
        const auto exact = number_traits<number_type>::from_chars(lexeme_);
        if (exact && std::isfinite(d)) {
            std::array<char, 32> buffer{};
            const auto denoted = number_traits<number_type>::from_chars(shortest_text(d, buffer));
            if (denoted && *denoted == *exact) {
                forget();
                return;
            }
        }
        const typename exact_table<Traits>::entry recorded{
            .value = exact ? *exact : number_type{},
            .representable = exact.has_value(),
            .read_as = d,
        };
        table_->insert(node_of_current(true), recorded);
    }

    json_resource scratch_;
    boost::json::value_stack stack_;
    std::pmr::vector<level> path_;
    std::pmr::string key_;
    std::pmr::string lexeme_;
    std::pmr::string token_;
    exact_table<Traits>* table_;
};

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_JSON_EXACT_HANDLER_HPP
