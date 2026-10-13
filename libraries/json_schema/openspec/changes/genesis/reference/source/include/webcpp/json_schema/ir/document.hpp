// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The IR's document: the tables that hold a schema's nodes, keywords, strings, values and numbers,
 how the loader and code build them, and the check of the IR's invariants.

 @see "The IR", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_IR_DOCUMENT_HPP
#define WEBCPP_JSON_SCHEMA_IR_DOCUMENT_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/json/deep_copy.hpp>
#include <webcpp/json_schema/detail/json/utf8.hpp>
#include <webcpp/json_schema/detail/json/value_arena.hpp>
#include <webcpp/json_schema/detail/numbers/compare.hpp>
#include <webcpp/json_schema/detail/numbers/decompose.hpp>
#include <webcpp/json_schema/errors.hpp>
#include <webcpp/json_schema/ir/ids.hpp>
#include <webcpp/json_schema/ir/keyword.hpp>
#include <webcpp/json_schema/ir/node.hpp>
#include <webcpp/json_schema/ir/number.hpp>
#include <webcpp/json_schema/numbers/number.hpp>
#include <webcpp/json_schema/traits.hpp>

#include <boost/assert.hpp>
#include <boost/container/deque.hpp>
#include <boost/container/map.hpp>
#include <boost/json/value.hpp>
#include <boost/throw_exception.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace webcpp::json_schema::ir {

/**
 A schema as the IR holds it: its nodes, their keywords, and the side tables the keywords name by
 index.

 Each table is a vector of the traits' allocator, and every part of the IR names another by its
 index, never by its address (invariant 13), so a copy with another allocator copies every table
 and names the same parts by the same values. The two exceptions are caches that the copy rebuilds:
 the views that deduplicate the interned strings, and the address each value id resolves to.

 A keyword's JSON value is held once: @ref add_value copies a value into the document's storage,
 iteratively, and @ref add_value_at names a value inside one already held, at a JSON Pointer,
 copying nothing. The storage frees its values all at once, so a value nested however deep dies
 without recursion.

 A JSON Pointer is held as steps (@ref pointer_step), each a token and the pointer it extends, so
 the pointers of a document hold each token once, and their memory is linear in the schema however
 long its keys and however deep its nesting. @ref write_pointer spells one where it is reported.

 The building members record what they are given, as given: a node defined twice, an index past
 its table or a pointer that resolves to nothing is kept, and @ref check reports it.

 @tparam Traits The traits: its `number_type` holds the numbers that no 64-bit integer holds, its
 `allocator_type` allocates every table.
 @see "The IR", in the guide.
*/
template <traits Traits>
class basic_document final {
public:
    /** The type of the numbers that no `std::int64_t` or `std::uint64_t` holds. */
    using number_type = Traits::number_type;

    /** The allocator of every table and of the values' storage. */
    using allocator_type = Traits::allocator_type;

    /**
     An empty document, whose tables allocate through a default allocator.

     Tip: it is a constructor of its own, not a default argument, so that an allocator that
     cannot be made without arguments leaves the type whole.
    */
    basic_document()
        requires std::default_initializable<allocator_type>
        : basic_document(allocator_type{}) {}

    /**
     An empty document, whose tables allocate through `allocator`.

     It allocates nothing until something is added.

     @param allocator The allocator of the tables and of the values' storage.
    */
    explicit basic_document(allocator_type allocator)
        : allocator_(allocator),
          nodes_(rebound<node>()),
          defined_(rebound<bool>()),
          keywords_(rebound<keyword>()),
          node_lists_(rebound<node_id>()),
          entries_(rebound<named_subschema>()),
          string_lists_(rebound<string_id>()),
          dependencies_(rebound<dependency>()),
          value_lists_(rebound<value_id>()),
          numbers_(rebound<ir::number<number_type>>()),
          resources_(rebound<resource>()),
          anchors_(rebound<anchor>()),
          steps_(rebound<pointer_step>()),
          strings_(rebound<string_type>()),
          string_index_(rebound<string_index_entry>()),
          owned_(rebound<boost::json::value>()),
          values_(rebound<value_entry>()),
          addresses_(rebound<const boost::json::value*>()),
          chain_(rebound<pointer_id>()) {}

    /**
     A copy of `other` whose every table, string and value is allocated through `allocator`
     (invariant 13).

     The values are copied iteratively, each value `other` owns once; a value named inside one is
     named again inside the copy, and its address resolved again there.

     @param other The document copied.
     @param allocator The allocator of the copy.
     @throws std::bad_alloc When the allocator throws it; without exceptions, the program ends.
    */
    basic_document(const basic_document& other, allocator_type allocator)
        : allocator_(allocator),
          root_(other.root_),
          redefined_(other.redefined_),
          defined_unreserved_(other.defined_unreserved_),
          nodes_(other.nodes_, rebound<node>()),
          defined_(other.defined_, rebound<bool>()),
          keywords_(other.keywords_, rebound<keyword>()),
          node_lists_(other.node_lists_, rebound<node_id>()),
          entries_(other.entries_, rebound<named_subschema>()),
          string_lists_(other.string_lists_, rebound<string_id>()),
          dependencies_(other.dependencies_, rebound<dependency>()),
          value_lists_(other.value_lists_, rebound<value_id>()),
          numbers_(other.numbers_, rebound<ir::number<number_type>>()),
          resources_(other.resources_, rebound<resource>()),
          anchors_(other.anchors_, rebound<anchor>()),
          steps_(other.steps_, rebound<pointer_step>()),
          strings_(rebound<string_type>()),
          string_index_(rebound<string_index_entry>()),
          owned_(rebound<boost::json::value>()),
          values_(other.values_, rebound<value_entry>()),
          addresses_(rebound<const boost::json::value*>()),
          chain_(rebound<pointer_id>()) {
        copy_strings(other);
        copy_values(other);
    }

    /**
     Takes every table and the values' storage of `other`, which holds none after.

     Tip: no string and no value moves in memory: the cached views and addresses stay valid.
    */
    basic_document(basic_document&&) noexcept = default;

    /**
     Frees this document, then takes every table, the values' storage and the allocator of `other`.

     Tip: the members cannot be assigned one by one: the values' storage would die before the
     values in it, and a table whose allocator differs would copy its elements, which may throw.

     @param other The document taken.
     @return This document.
    */
    basic_document& operator=(basic_document&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        std::destroy_at(this);
        std::construct_at(this, std::move(other));
        return *this;
    }

    /** Not copied without an allocator: the copy names the allocator it allocates through. */
    basic_document(const basic_document&) = delete;

    /** Not copied, as the copy constructor is not. */
    basic_document& operator=(const basic_document&) = delete;

    /** Frees the values, then their storage, then every table. */
    ~basic_document() = default;

    /**
     The root node.

     @return The node @ref set_root named, or @ref no_node before.
    */
    [[nodiscard]] node_id root() const noexcept { return root_; }

    /**
     A node.

     @param id The node, which the document holds.
     @return The node.
    */
    [[nodiscard]] const node& at(node_id id) const noexcept {
        BOOST_ASSERT(index(id) < nodes_.size());
        return nodes_[index(id)];
    }

    /**
     The keywords of a node, in source order.

     @param id The node, which the document holds.
     @return Its keywords; none before it is defined.
    */
    [[nodiscard]] std::span<const keyword> keywords(node_id id) const noexcept {
        return slice(keywords_, at(id).keywords);
    }

    /**
     The keyword of a kind in a node.

     @param id The node, which the document holds.
     @param kind The kind.
     @return The node's first keyword of `kind`, or nullptr when it has none.
    */
    [[nodiscard]] const keyword* find(node_id id, keyword_kind kind) const noexcept {
        for (const keyword& k : keywords(id)) {
            if (k.kind == kind) {
                return &k;
            }
        }
        return nullptr;
    }

    /**
     A slice of the node lists.

     @param r The slice, inside the table.
     @return The nodes.
    */
    [[nodiscard]] std::span<const node_id> nodes(range r) const noexcept {
        return slice(node_lists_, r);
    }

    /**
     A slice of the named subschemas.

     @param r The slice, inside the table.
     @return The entries.
    */
    [[nodiscard]] std::span<const named_subschema> entries(range r) const noexcept {
        return slice(entries_, r);
    }

    /**
     A slice of the string lists.

     @param r The slice, inside the table.
     @return The strings.
    */
    [[nodiscard]] std::span<const string_id> strings(range r) const noexcept {
        return slice(string_lists_, r);
    }

    /**
     A slice of the dependencies.

     @param r The slice, inside the table.
     @return The dependencies.
    */
    [[nodiscard]] std::span<const dependency> dependencies(range r) const noexcept {
        return slice(dependencies_, r);
    }

    /**
     A slice of the value lists.

     @param r The slice, inside the table.
     @return The values.
    */
    [[nodiscard]] std::span<const value_id> values(range r) const noexcept {
        return slice(value_lists_, r);
    }

    /**
     An interned string.

     @param id The string, which the document holds.
     @return Its text, valid as long as the document, moves included.
    */
    [[nodiscard]] std::string_view string(string_id id) const noexcept {
        BOOST_ASSERT(index(id) < strings_.size());
        return strings_[index(id)];
    }

    /**
     A JSON value.

     @param id The value, which the document holds and whose pointer resolves.
     @return The value, in the document's storage, valid as long as the document, moves included.
    */
    [[nodiscard]] const boost::json::value& value(value_id id) const noexcept {
        BOOST_ASSERT(index(id) < addresses_.size() && addresses_[index(id)] != nullptr);
        return *addresses_[index(id)];
    }

    /**
     A number operand.

     @param id The number, which the document holds.
     @return The number.
    */
    [[nodiscard]] const ir::number<number_type>& number(number_id id) const noexcept {
        BOOST_ASSERT(index(id) < numbers_.size());
        return numbers_[index(id)];
    }

    /**
     The schema resources; the root's is the first.

     @return The resources, by @ref resource_id.
    */
    [[nodiscard]] std::span<const resource> resources() const noexcept { return resources_; }

    /**
     The anchors.

     @return The anchors, in the order they were added.
    */
    [[nodiscard]] std::span<const anchor> anchors() const noexcept { return anchors_; }

    /**
     A step of a JSON Pointer.

     @param id The pointer, which the document holds and which is not @ref empty_pointer.
     @return Its last step.
    */
    [[nodiscard]] const pointer_step& step(pointer_id id) const noexcept {
        BOOST_ASSERT(index(id) < steps_.size());
        return steps_[index(id)];
    }

    /**
     Appends a JSON Pointer, spelled as RFC 6901 writes it, to `out`: each step a slash, then the
     member's name with `~` written `~0` and `/` written `~1`, or the element's index.

     It reads each step twice, once to size `out` and once to write it, and allocates only what
     `out` grows by. A step the document does not hold, or whose parent does not come before it,
     ends the pointer there.

     @param id The pointer.
     @param out The string appended to: `size`, `resize` and `operator[]`, a `basic_string` or a
     `static_string`.
     @throws std::bad_alloc When `out` cannot grow; without exceptions, the program ends.
    */
    template <class String>
    void write_pointer(pointer_id id, String& out) const {
        std::size_t length = 0;
        for (pointer_id at = id; follows(at); at = steps_[index(at)].parent) {
            length += 1 + token_length(steps_[index(at)]);
        }
        const std::size_t start = out.size();
        out.resize(start + length);
        std::size_t end = start + length;
        for (pointer_id at = id; follows(at); at = steps_[index(at)].parent) {
            const pointer_step& s = steps_[index(at)];
            std::size_t next = end - token_length(s);
            end = next - 1;
            out[end] = '/';
            if (s.element) {
                std::array<char, 16> digits{};
                const auto written =
                    std::to_chars(digits.data(), digits.data() + digits.size(), s.index);
                for (const char* c = digits.data(); c != written.ptr; ++c) {
                    out[next++] = *c;
                }
                continue;
            }
            for (const char c : string(s.name)) {
                if (c == '~' || c == '/') {
                    out[next++] = '~';
                    out[next++] = c == '~' ? '0' : '1';
                } else {
                    out[next++] = c;
                }
            }
        }
    }

    /**
     How many nodes the document holds, defined or only reserved.

     @return The count of nodes.
    */
    [[nodiscard]] std::size_t size() const noexcept { return nodes_.size(); }

    /**
     Adds a node, to be defined later, so that a parent's keywords can name it first.

     @param resource The resource it belongs to.
     @param pointer Its JSON Pointer from the resource's root: @ref empty_pointer for the root.
     @return The node.
     @throws std::bad_alloc When the allocator throws it; std::length_error past 2^32 - 1 nodes.
    */
    node_id reserve(resource_id resource, pointer_id pointer) {
        const node_id id{next_index(nodes_.size(), 1)};
        make_room(nodes_);
        make_room(defined_);
        nodes_.push_back({
            .kind = node_kind::never_valid,
            .resource = resource,
            .pointer = pointer,
            .keywords = {},
        });
        defined_.push_back(false);
        return id;
    }

    /**
     Defines a reserved node: its kind and its keywords, appended to the keyword table.

     A node is defined once. A second definition, and one of a node never reserved, changes
     nothing, and @ref check reports it (invariant 1).

     @param id The node.
     @param kind What it is.
     @param keywords Its keywords, in source order.
     @throws std::bad_alloc When the allocator throws it; std::length_error past 2^32 - 1 keywords.
    */
    void define(node_id id, node_kind kind, std::span<const keyword> keywords) {
        if (index(id) >= nodes_.size()) {
            defined_unreserved_ = true;
            return;
        }
        if (defined_[index(id)]) {
            redefined_ = true;
            return;
        }
        const range added = append(keywords_, keywords);
        nodes_[index(id)].kind = kind;
        nodes_[index(id)].keywords = added;
        defined_[index(id)] = true;
    }

    /**
     Names the root node.

     @param id The root.
    */
    void set_root(node_id id) noexcept { root_ = id; }

    /**
     Adds a schema resource; the first is the root's.

     @param r The resource.
     @return Its index.
     @throws std::bad_alloc When the allocator throws it; std::length_error past 2^32 - 1.
    */
    resource_id add_resource(const resource& r) {
        const resource_id id{next_index(resources_.size(), 1)};
        resources_.push_back(r);
        return id;
    }

    /**
     Adds an anchor.

     @param a The anchor.
     @throws std::bad_alloc When the allocator throws it.
    */
    void add_anchor(const anchor& a) { anchors_.push_back(a); }

    /**
     A string held by the document: the one already held with the same bytes, or a copy.

     The strings are deduplicated by an ordered index, never a hash table, so that text chosen to
     collide costs what any other text costs.

     @param text The text.
     @return The string; equal texts have equal ids.
     @throws std::bad_alloc When the allocator throws it; std::length_error past 2^32 - 1 strings.
    */
    string_id intern(std::string_view text) {
        if (const auto found = string_index_.find(text); found != string_index_.end()) {
            return found->second;
        }
        const string_id id{next_index(strings_.size(), 1)};
        strings_.push_back(string_type(text, rebound<char>()));
        string_index_.emplace(std::string_view(strings_.back()), id);
        return id;
    }

    /**
     Adds a JSON Pointer: `parent`, then the member `name`.

     @param parent The pointer it extends, which the document holds, or @ref empty_pointer.
     @param name The member's name, as written, not escaped.
     @return The pointer.
     @throws std::bad_alloc When the allocator throws it; std::length_error past 2^32 - 1 steps.
    */
    pointer_id add_pointer(pointer_id parent, string_id name) {
        const pointer_id id{next_index(steps_.size(), 1)};
        steps_.push_back({.parent = parent, .name = name, .index = 0, .element = false});
        return id;
    }

    /**
     Adds a JSON Pointer: `parent`, then the element `element`.

     @param parent The pointer it extends, which the document holds, or @ref empty_pointer.
     @param element The element's index.
     @return The pointer.
     @throws std::bad_alloc When the allocator throws it; std::length_error past 2^32 - 1 steps.
    */
    pointer_id add_pointer(pointer_id parent, std::uint32_t element) {
        const pointer_id id{next_index(steps_.size(), 1)};
        steps_.push_back({.parent = parent, .name = {}, .index = element, .element = true});
        return id;
    }

    /**
     Copies a JSON value into the document's storage, iteratively, once.

     @param v The value.
     @return The value's id.
     @throws std::bad_alloc When the allocator throws it; std::length_error past 2^32 - 1 values.
    */
    value_id add_value(const boost::json::value& v) {
        const value_id id{next_index(values_.size(), 1)};
        detail::value_arena<allocator_type>& storage = arena();
        owned_.push_back(detail::deep_copy(v, storage.storage(), storage.resource()));
        make_room(values_);
        make_room(addresses_);
        values_.push_back({.base = id, .pointer = empty_pointer});
        addresses_.push_back(&owned_.back());
        return id;
    }

    /**
     Names a value inside one the document holds, at a JSON Pointer from it, copying nothing.

     `base` is a value the document owns or one it names so: a value named one step inside
     another costs one step to resolve, however deep the two lie in the value that holds them.

     @param base The value pointed into, which comes before this one.
     @param pointer The JSON Pointer from `base`.
     @return The value's id; when `base` is not the document's or `pointer` resolves to nothing,
     the id is kept and @ref check reports it (invariants 1 and 11).
     @throws std::bad_alloc When the allocator throws it; std::length_error past 2^32 - 1 values.
    */
    value_id add_value_at(value_id base, pointer_id pointer) {
        const value_id id{next_index(values_.size(), 1)};
        const std::size_t b = index(base);
        const boost::json::value* at = nullptr;
        if (b < addresses_.size() && addresses_[b] != nullptr) {
            at = resolve(*addresses_[b], pointer, chain_);
        }
        make_room(values_);
        make_room(addresses_);
        values_.push_back({.base = base, .pointer = pointer});
        addresses_.push_back(at);
        return id;
    }

    /**
     Adds a number operand.

     @param n The number.
     @return Its id.
     @throws std::bad_alloc When the allocator throws it; std::length_error past 2^32 - 1 numbers.
    */
    number_id add_number(const ir::number<number_type>& n) {
        const number_id id{next_index(numbers_.size(), 1)};
        numbers_.push_back(n);
        return id;
    }

    /**
     Appends nodes to the node lists.

     @param ids The nodes.
     @return Their slice.
     @throws std::bad_alloc When the allocator throws it; std::length_error past 2^32 - 1.
    */
    range add_nodes(std::span<const node_id> ids) { return append(node_lists_, ids); }

    /**
     Appends named subschemas to their table.

     @param entries The entries, in source order.
     @return Their slice.
     @throws std::bad_alloc When the allocator throws it; std::length_error past 2^32 - 1.
    */
    range add_entries(std::span<const named_subschema> entries) {
        return append(entries_, entries);
    }

    /**
     Appends strings to the string lists.

     @param ids The strings.
     @return Their slice.
     @throws std::bad_alloc When the allocator throws it; std::length_error past 2^32 - 1.
    */
    range add_strings(std::span<const string_id> ids) { return append(string_lists_, ids); }

    /**
     Appends dependencies to their table.

     @param deps The dependencies, in source order.
     @return Their slice.
     @throws std::bad_alloc When the allocator throws it; std::length_error past 2^32 - 1.
    */
    range add_dependencies(std::span<const dependency> deps) { return append(dependencies_, deps); }

    /**
     Appends values to the value lists.

     @param ids The values.
     @return Their slice.
     @throws std::bad_alloc When the allocator throws it; std::length_error past 2^32 - 1.
    */
    range add_values(std::span<const value_id> ids) { return append(value_lists_, ids); }

    /**
     Checks invariants 1 to 11, 13 and 15, and returns the first fault found.

     The strings are checked first, so that every text a fault names is UTF-8; then the pointers'
     steps, so that every pointer a fault names can be spelled; then the nodes and their keywords,
     the values, the resources and the anchors. Invariant 13 holds by
     construction: the tables hold indices only. Nothing recurses: every walk is a loop.

     @return Nothing, or a diagnostic of @ref errc::invalid_ir whose `what` starts with the
     invariant, as in `"[ir-4] minLength"`, and whose `where` is the node's pointer or the table.
     @throws std::bad_alloc When the allocator throws it, for the scratch that finds duplicates and
     spells pointers.
    */
    [[nodiscard]] result<void> check() const {
        if (root_ == no_node || index(root_) >= nodes_.size()) {
            return fault(1, "root", "the root is not set");
        }
        if (redefined_) {
            return fault(1, "nodes", "a node is defined twice");
        }
        if (defined_unreserved_) {
            return fault(1, "nodes", "a node never reserved is defined");
        }
        if (resources_.empty() || resources_[0].root != root_) {
            return fault(9, "resources", "resource 0 is not the root's");
        }
        if (auto r = check_strings(); !r) {
            return r;
        }
        if (auto r = check_steps(); !r) {
            return r;
        }
        scratch_type scratch(rebound<std::uint32_t>());
        string_type where(rebound<char>());
        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            if (auto r = check_node(i, scratch); !r) {
                return r;
            }
        }
        if (auto r = check_values(where); !r) {
            return r;
        }
        if (auto r = check_resources(scratch); !r) {
            return r;
        }
        return check_anchors(scratch);
    }

    /**
     The allocator.

     @return A copy of the allocator every table allocates through.
    */
    [[nodiscard]] allocator_type get_allocator() const noexcept { return allocator_; }

private:
    template <class T>
    using rebind = std::allocator_traits<allocator_type>::template rebind_alloc<T>;

    template <class T>
    using table = std::vector<T, rebind<T>>;

    using string_type = basic_string<Traits>;

    using string_index_entry = std::pair<const std::string_view, string_id>;

    using scratch_type = table<std::uint32_t>;

    /** A value: the owned value it is in, and its JSON Pointer there; owned when `base` is it. */
    struct value_entry {
        /** The value it is in, owned or named, which comes before it: itself when it is owned. */
        value_id base{};

        /** Its JSON Pointer from `base`'s value. */
        pointer_id pointer = empty_pointer;
    };

    /** The allocator, rebound to `T`. */
    template <class T>
    [[nodiscard]] rebind<T> rebound() const noexcept {
        return rebind<T>(allocator_);
    }

    /** The index an id holds. */
    template <class Id>
    static constexpr std::size_t index(Id id) noexcept {
        return static_cast<std::size_t>(id);
    }

    /**
     The index of the first of `added` entries appended to a table of `size`.

     Tip: 2^32 - 1 is never an index: it is @ref no_node and @ref no_value.
    */
    static std::uint32_t next_index(std::size_t size, std::size_t added) {
        constexpr std::size_t limit = 0xFFFF'FFFFU;
        if (added > limit || size > limit - added) {
            boost::throw_exception(std::length_error("webcpp::json_schema::ir: a table is full"));
        }
        return static_cast<std::uint32_t>(size);
    }

    /**
     Grows `t` geometrically when it is full, so that the push that follows does not allocate.

     Tip: two tables that grow together (a node and its defined bit, a value and its address) both
     make room first, so a failed allocation leaves neither one longer than the other.
    */
    template <class T>
    static void make_room(table<T>& t) {
        if (t.size() == t.capacity()) {
            t.reserve(std::max<std::size_t>(8, 2 * t.capacity()));
        }
    }

    /** Appends `items` to `t`, and returns their slice. */
    template <class T>
    static range append(table<T>& t, std::span<const T> items) {
        const std::uint32_t first = next_index(t.size(), items.size());
        t.insert(t.end(), items.begin(), items.end());
        return {.first = first, .count = static_cast<std::uint32_t>(items.size())};
    }

    /** Whether the slice `r` lies inside `t`. */
    template <class T>
    static bool inside(const table<T>& t, range r) noexcept {
        return r.first <= t.size() && r.count <= t.size() - r.first;
    }

    /** The entries of `t` the slice `r` names, which lies inside it. */
    template <class T>
    static std::span<const T> slice(const table<T>& t, range r) noexcept {
        BOOST_ASSERT(inside(t, r));
        return {t.data() + r.first, r.count};
    }

    /** The values' storage, made on the first value added. */
    detail::value_arena<allocator_type>& arena() {
        if (!arena_) {
            arena_ =
                std::allocate_shared<detail::value_arena<allocator_type>>(allocator_, allocator_);
        }
        return *arena_;
    }

    /**
     Whether the walk up a pointer goes on at `at`: a step the document holds, whose name it holds
     too, after its parent. Spelling a pointer and resolving one both stop where it does not.
    */
    [[nodiscard]] bool follows(pointer_id at) const noexcept {
        if (at == empty_pointer || index(at) >= steps_.size()) {
            return false;
        }
        const pointer_step& s = steps_[index(at)];
        return (s.element || holds(s.name)) &&
               (s.parent == empty_pointer || index(s.parent) < index(at));
    }

    /** The length of a step's token, escaped: `~` and `/` take two bytes each. */
    [[nodiscard]] std::size_t token_length(const pointer_step& s) const noexcept {
        if (s.element) {
            std::array<char, 16> digits{};
            const auto written =
                std::to_chars(digits.data(), digits.data() + digits.size(), s.index);
            return static_cast<std::size_t>(written.ptr - digits.data());
        }
        const std::string_view name = string(s.name);
        return name.size() +
               static_cast<std::size_t>(std::count_if(name.begin(), name.end(),
                                                      [](char c) { return c == '~' || c == '/'; }));
    }

    /** One step down from `v`: its member or its element that `s` names, or nullptr. */
    [[nodiscard]] const boost::json::value* child(const boost::json::value& v,
                                                  const pointer_step& s) const {
        if (s.element) {
            const auto* array = v.if_array();
            return array != nullptr && s.index < array->size() ? &(*array)[s.index] : nullptr;
        }
        const auto* object = v.if_object();
        return object != nullptr && holds(s.name) ? object->if_contains(string(s.name)) : nullptr;
    }

    /**
     The value `pointer` names inside `base`, or nullptr: a step the document does not hold, a
     parent after its step, or a token that names nothing.

     Tip: the steps are gathered in `chain`, from the last, and then followed from the first, each
     looked up by Boost.JSON's index of its object, so a pointer costs its steps, never a scan of an
     object's keys.
    */
    [[nodiscard]] const boost::json::value* resolve(const boost::json::value& base,
                                                    pointer_id pointer,
                                                    table<pointer_id>& chain) const {
        chain.clear();
        for (pointer_id at = pointer; at != empty_pointer; at = steps_[index(at)].parent) {
            if (!follows(at)) {
                return nullptr;
            }
            chain.push_back(at);
        }
        const boost::json::value* v = &base;
        for (auto at = chain.rbegin(); at != chain.rend() && v != nullptr; ++at) {
            v = child(*v, steps_[index(*at)]);
        }
        return v;
    }

    /** Copies the strings of `other` with this allocator, and indexes them as `other` does. */
    void copy_strings(const basic_document& other) {
        for (const string_type& s : other.strings_) {
            strings_.push_back(string_type(s, rebound<char>()));
        }
        for (const string_index_entry& entry : other.string_index_) {
            string_index_.emplace_hint(
                string_index_.end(), std::string_view(strings_[index(entry.second)]), entry.second);
        }
    }

    /**
     Copies each value `other` owns into this document's storage, in id order, and resolves each
     value named inside another again, here, from that other's address.

     Tip: a value named inside another always comes after it, since the other existed first, so
     one pass in id order finds every base already copied.
    */
    void copy_values(const basic_document& other) {
        addresses_.reserve(values_.size());
        for (std::size_t i = 0; i < values_.size(); ++i) {
            const std::size_t b = index(values_[i].base);
            if (other.owns(i)) {
                detail::value_arena<allocator_type>& storage = arena();
                owned_.push_back(
                    detail::deep_copy(*other.addresses_[i], storage.storage(), storage.resource()));
                addresses_.push_back(&owned_.back());
                continue;
            }
            const boost::json::value* at = nullptr;
            if (b < i && addresses_[b] != nullptr) {
                at = resolve(*addresses_[b], values_[i].pointer, chain_);
            }
            addresses_.push_back(at);
        }
    }

    /**
     Whether the value at index `i` is one the document owns: its own base, with an address.

     Tip: a value named inside the id it was about to receive is its own base too, but has no
     address, and is owned by nobody.
    */
    [[nodiscard]] bool owns(std::size_t i) const noexcept {
        return index(values_[i].base) == i && addresses_[i] != nullptr;
    }

    /** Whether the document holds the node `id`. */
    [[nodiscard]] bool holds(node_id id) const noexcept { return index(id) < nodes_.size(); }

    /** Whether the document holds the string `id`. */
    [[nodiscard]] bool holds(string_id id) const noexcept { return index(id) < strings_.size(); }

    /** Whether the document holds the value `id`. */
    [[nodiscard]] bool holds(value_id id) const noexcept { return index(id) < values_.size(); }

    /** Whether the document holds the pointer `id`: the empty one, or a step of its table. */
    [[nodiscard]] bool holds(pointer_id id) const noexcept {
        return id == empty_pointer || index(id) < steps_.size();
    }

    /**
     The diagnostic of a broken invariant: `what` is `"[ir-<invariant>] <what>"`, then `": "` and
     `name` when there is one.

     Tip: the text is formatted into a fixed buffer, longer than a diagnostic holds, so that the
     diagnostic's own cut, on a code point boundary, is the one that applies.
    */
    static diagnostic fault(std::uint32_t invariant, std::string_view where, std::string_view what,
                            std::string_view name = {}) noexcept {
        std::array<char, 300> text{};
        std::size_t used = 0;
        const auto put = [&](std::string_view part) {
            const std::size_t n = std::min(part.size(), text.size() - used);
            std::copy_n(part.begin(), n, text.begin() + static_cast<std::ptrdiff_t>(used));
            used += n;
        };
        std::array<char, 8> digits{};
        const auto written = std::to_chars(digits.data(), digits.data() + digits.size(), invariant);
        put("[ir-");
        put(std::string_view(digits.data(), static_cast<std::size_t>(written.ptr - digits.data())));
        put("] ");
        put(what);
        if (!name.empty()) {
            put(": ");
            put(name);
        }
        return detail::make_diagnostic(make_error_code(errc::invalid_ir), where,
                                       std::string_view(text.data(), used));
    }

    /**
     The diagnostic of a broken invariant at the node whose pointer is `where`, spelled here, for
     the one fault a check returns.
    */
    [[nodiscard]] diagnostic fault(std::uint32_t invariant, pointer_id where, std::string_view what,
                                   std::string_view name = {}) const {
        string_type text(rebound<char>());
        write_pointer(where, text);
        return fault(invariant, std::string_view(text), what, name);
    }

    /**
     The first duplicate among `ids` once sorted by `less`, or nothing.

     Tip: equal texts have equal string ids, so a sort of ids finds equal names in O(n log n).
    */
    template <class Less>
    static std::optional<std::uint32_t> duplicate(scratch_type& ids, Less less) {
        std::sort(ids.begin(), ids.end(), less);
        const auto found = std::adjacent_find(
            ids.begin(), ids.end(), [&](std::uint32_t a, std::uint32_t b) { return !less(a, b); });
        if (found == ids.end()) {
            return std::nullopt;
        }
        return *found;
    }

    /** Invariant 8: every string is UTF-8. */
    [[nodiscard]] result<void> check_strings() const {
        for (const string_type& s : strings_) {
            if (!detail::is_valid_utf8(s)) {
                return fault(8, "strings", "a string is not UTF-8");
            }
        }
        return {};
    }

    /**
     Invariants 1 and 15: each step of a pointer extends the empty pointer or a step before it, and
     names its member by a string of the document.
    */
    [[nodiscard]] result<void> check_steps() const {
        for (std::size_t i = 0; i < steps_.size(); ++i) {
            const pointer_step& s = steps_[i];
            if (s.parent != empty_pointer && index(s.parent) >= i) {
                return fault(15, "pointers",
                             "a step extends a pointer that does not come before it");
            }
            if (!s.element && !holds(s.name)) {
                return fault(1, "pointers", "a step's name is not a string of the document");
            }
        }
        return {};
    }

    /**
     Invariants 1, 4 and the payloads' of the node at index `i`, whose pointer a fault names.

     Tip: the pointer is spelled only for the fault returned, never for a node that has none, so a
     check reads each node once, however long its pointer.
    */
    [[nodiscard]] result<void> check_node(std::size_t i, scratch_type& scratch) const {
        const node& n = nodes_[i];
        if (!holds(n.pointer)) {
            return fault(1, "nodes", "a node's pointer is not a pointer of the document");
        }
        const pointer_id where = n.pointer;
        if (!defined_[i]) {
            return fault(1, where, "a reserved node is never defined");
        }
        if (index(n.resource) >= resources_.size()) {
            return fault(1, where, "the node's resource does not exist");
        }
        if (!inside(keywords_, n.keywords)) {
            return fault(1, where, "the node's keywords are not in the keyword table");
        }
        std::array<bool, 256> seen{};
        for (const keyword& k : slice(keywords_, n.keywords)) {
            if (auto r = check_keyword(k, where, seen, scratch); !r) {
                return r;
            }
        }
        return check_repeated_names(n, where, scratch);
    }

    /** Invariants 1 and 4, then the payload's, of one keyword of the node at `where`. */
    [[nodiscard]] result<void> check_keyword(const keyword& k, pointer_id where,
                                             std::array<bool, 256>& seen,
                                             scratch_type& scratch) const {
        if (!holds(k.name)) {
            return fault(1, where, "a keyword's name is not a string of the document");
        }
        const std::string_view name = string(k.name);
        const std::size_t expected = payload_index(k.kind);
        if (expected == 0) {
            return fault(1, where, "a keyword of no kind", name);
        }
        if (k.data.index() != expected) {
            return fault(1, where, "a payload that is not its kind's", name);
        }
        if (k.source != no_value && !holds(k.source)) {
            return fault(1, where, "a source that is not a value of the document", name);
        }
        const auto kind = static_cast<std::size_t>(k.kind);
        if (seen[kind] && !repeats(k.kind)) {
            return fault(4, where, "a keyword kind twice in a node", name);
        }
        seen[kind] = true;
        return std::visit(
            [&](const auto& part) { return check_part(part, k.kind, where, name, scratch); },
            k.data);
    }

    /** Whether keywords of `kind` may repeat in a node, each with a name of its own. */
    static bool repeats(keyword_kind kind) noexcept {
        return kind == keyword_kind::unknown || kind == keyword_kind::ignored ||
               kind == keyword_kind::user;
    }

    /** Invariant 4: the keywords of a kind that repeats have distinct names in the node `n`. */
    [[nodiscard]] result<void> check_repeated_names(const node& n, pointer_id where,
                                                    scratch_type& scratch) const {
        scratch.clear();
        for (std::uint32_t k = n.keywords.first; k < n.keywords.first + n.keywords.count; ++k) {
            if (repeats(keywords_[k].kind)) {
                scratch.push_back(k);
            }
        }
        const auto twice = duplicate(scratch, [&](std::uint32_t a, std::uint32_t b) {
            return std::pair(keywords_[a].kind, keywords_[a].name) <
                   std::pair(keywords_[b].kind, keywords_[b].name);
        });
        if (twice) {
            return fault(4, where, "a keyword that repeats has one name twice",
                         string(keywords_[*twice].name));
        }
        return {};
    }

    /** Invariant 1: `id` is a node of the document. */
    [[nodiscard]] result<void> check_schema(node_id id, pointer_id where,
                                            std::string_view name) const {
        if (!holds(id)) {
            return fault(1, where, "a subschema that is not a node of the document", name);
        }
        return {};
    }

    /** Invariant 1: `id` is a string of the document. */
    [[nodiscard]] result<void> check_text(string_id id, pointer_id where,
                                          std::string_view name) const {
        if (!holds(id)) {
            return fault(1, where, "a string that is not one of the document", name);
        }
        return {};
    }

    /** Invariant 1: every node of a list is the document's. */
    [[nodiscard]] result<void> check_schemas(range r, pointer_id where,
                                             std::string_view name) const {
        if (!inside(node_lists_, r)) {
            return fault(1, where, "a list of subschemas outside its table", name);
        }
        for (const node_id id : slice(node_lists_, r)) {
            if (auto checked = check_schema(id, where, name); !checked) {
                return checked;
            }
        }
        return {};
    }

    /** Invariant 1: every string of a list is the document's. */
    [[nodiscard]] result<void> check_texts(range r, pointer_id where, std::string_view name) const {
        if (!inside(string_lists_, r)) {
            return fault(1, where, "a list of strings outside its table", name);
        }
        for (const string_id id : slice(string_lists_, r)) {
            if (auto checked = check_text(id, where, name); !checked) {
                return checked;
            }
        }
        return {};
    }

    /** No payload: no kind holds one, which @ref check_keyword refuses first. */
    static result<void> check_part(std::monostate /*part*/, keyword_kind /*kind*/,
                                   pointer_id /*where*/, std::string_view /*name*/,
                                   scratch_type& /*scratch*/) noexcept {
        return {};
    }

    /** One subschema. */
    [[nodiscard]] result<void> check_part(const subschema& part, keyword_kind /*kind*/,
                                          pointer_id where, std::string_view name,
                                          scratch_type& /*scratch*/) const {
        return check_schema(part.schema, where, name);
    }

    /** A list of subschemas. */
    [[nodiscard]] result<void> check_part(const subschema_list& part, keyword_kind /*kind*/,
                                          pointer_id where, std::string_view name,
                                          scratch_type& /*scratch*/) const {
        return check_schemas(part.nodes, where, name);
    }

    /** Named subschemas, whose names are distinct (invariant 6). */
    [[nodiscard]] result<void> check_part(const named_subschemas& part, keyword_kind /*kind*/,
                                          pointer_id where, std::string_view name,
                                          scratch_type& scratch) const {
        if (!inside(entries_, part.entries)) {
            return fault(1, where, "named subschemas outside their table", name);
        }
        scratch.clear();
        for (const named_subschema& entry : slice(entries_, part.entries)) {
            if (auto r = check_text(entry.name, where, name); !r) {
                return r;
            }
            if (auto r = check_schema(entry.schema, where, name); !r) {
                return r;
            }
            scratch.push_back(static_cast<std::uint32_t>(entry.name));
        }
        if (const auto twice = duplicate(scratch, std::less<>{})) {
            return fault(6, where, "an entry's name given twice", string(string_id{*twice}));
        }
        return {};
    }

    /** A reference, resolved (invariant 2). */
    [[nodiscard]] result<void> check_part(const reference& part, keyword_kind /*kind*/,
                                          pointer_id where, std::string_view name,
                                          scratch_type& /*scratch*/) const {
        if (part.target == no_node) {
            return fault(2, where, "a reference not resolved", name);
        }
        if (auto r = check_schema(part.target, where, name); !r) {
            return r;
        }
        return check_text(part.uri, where, name);
    }

    /** A dynamic reference, its initial target resolved (invariant 2). */
    [[nodiscard]] result<void> check_part(const dynamic_reference& part, keyword_kind /*kind*/,
                                          pointer_id where, std::string_view name,
                                          scratch_type& /*scratch*/) const {
        if (part.initial_target == no_node) {
            return fault(2, where, "a dynamic reference without its initial target", name);
        }
        if (auto r = check_schema(part.initial_target, where, name); !r) {
            return r;
        }
        if (auto r = check_text(part.uri, where, name); !r) {
            return r;
        }
        return check_text(part.anchor, where, name);
    }

    /** Types: nothing names another part. */
    static result<void> check_part(const types& /*part*/, keyword_kind /*kind*/,
                                   pointer_id /*where*/, std::string_view /*name*/,
                                   scratch_type& /*scratch*/) noexcept {
        return {};
    }

    /** One value. */
    [[nodiscard]] result<void> check_part(const constant& part, keyword_kind /*kind*/,
                                          pointer_id where, std::string_view name,
                                          scratch_type& /*scratch*/) const {
        if (!holds(part.value)) {
            return fault(1, where, "a value that is not one of the document", name);
        }
        return {};
    }

    /** A list of values. */
    [[nodiscard]] result<void> check_part(const constants& part, keyword_kind /*kind*/,
                                          pointer_id where, std::string_view name,
                                          scratch_type& /*scratch*/) const {
        if (!inside(value_lists_, part.values)) {
            return fault(1, where, "a list of values outside its table", name);
        }
        for (const value_id id : slice(value_lists_, part.values)) {
            if (!holds(id)) {
                return fault(1, where, "a value that is not one of the document", name);
            }
        }
        return {};
    }

    /**
     A number operand, exact (invariant 7): a finite `Number`, never one that is an integer
     `std::int64_t` or `std::uint64_t` holds, and above zero for `multipleOf`.
    */
    [[nodiscard]] result<void> check_part(const bound& part, keyword_kind kind, pointer_id where,
                                          std::string_view name, scratch_type& /*scratch*/) const {
        if (index(part.value) >= numbers_.size()) {
            return fault(1, where, "a number that is not one of the document", name);
        }
        const ir::number<number_type>& n = numbers_[index(part.value)];
        if (const auto* held = std::get_if<2>(&n); held != nullptr && !is_finite(*held)) {
            return fault(7, where, "a number that is not finite", name);
        }
        const decomposed exact = detail::decompose(n);
        if (n.index() == 2 && detail::integer_of<number_type>(exact)) {
            return fault(7, where, "an integer held as a Number", name);
        }
        if (kind == keyword_kind::multiple_of &&
            detail::compare(exact, decomposed{}) != std::strong_ordering::greater) {
            return fault(7, where, "a divisor that is not above zero", name);
        }
        return {};
    }

    /**
     Whether `n` is finite: its own text reads back as a JSON number, which neither a NaN nor an
     infinity is.

     Tip: the number concept has no finiteness query, and a decomposition cannot tell: Boost.Decimal
     decomposes a NaN and an infinity as its largest coefficient.
    */
    static bool is_finite(const number_type& n) {
        std::array<char, 128> text{};
        const std::size_t written = number_traits<number_type>::to_chars(text, n);
        return written != 0 &&
               number_traits<number_type>::from_chars(std::string_view(text.data(), written))
                   .has_value();
    }

    /** A count: nothing names another part. */
    static result<void> check_part(const count& /*part*/, keyword_kind /*kind*/,
                                   pointer_id /*where*/, std::string_view /*name*/,
                                   scratch_type& /*scratch*/) noexcept {
        return {};
    }

    /** A text. */
    [[nodiscard]] result<void> check_part(const text& part, keyword_kind /*kind*/, pointer_id where,
                                          std::string_view name, scratch_type& /*scratch*/) const {
        return check_text(part.value, where, name);
    }

    /** A format. */
    [[nodiscard]] result<void> check_part(const format_payload& part, keyword_kind /*kind*/,
                                          pointer_id where, std::string_view name,
                                          scratch_type& /*scratch*/) const {
        return check_text(part.name, where, name);
    }

    /** A list of names. */
    [[nodiscard]] result<void> check_part(const names& part, keyword_kind /*kind*/,
                                          pointer_id where, std::string_view name,
                                          scratch_type& /*scratch*/) const {
        return check_texts(part.strings, where, name);
    }

    /** Dependencies: each property and the names it requires. */
    [[nodiscard]] result<void> check_part(const ir::dependencies& part, keyword_kind /*kind*/,
                                          pointer_id where, std::string_view name,
                                          scratch_type& /*scratch*/) const {
        if (!inside(dependencies_, part.entries)) {
            return fault(1, where, "dependencies outside their table", name);
        }
        for (const dependency& d : slice(dependencies_, part.entries)) {
            if (auto r = check_text(d.name, where, name); !r) {
                return r;
            }
            if (auto r = check_texts(d.strings, where, name); !r) {
                return r;
            }
        }
        return {};
    }

    /** `contains`. */
    [[nodiscard]] result<void> check_part(const contains_payload& part, keyword_kind /*kind*/,
                                          pointer_id where, std::string_view name,
                                          scratch_type& /*scratch*/) const {
        return check_schema(part.schema, where, name);
    }

    /** A user keyword: nothing names another part. */
    static result<void> check_part(const user_payload& /*part*/, keyword_kind /*kind*/,
                                   pointer_id /*where*/, std::string_view /*name*/,
                                   scratch_type& /*scratch*/) noexcept {
        return {};
    }

    /**
     Invariants 1 and 11: each value is owned, or named inside a value before it, where it
     resolves; a pointer that resolves to nothing is spelled into `where`, which the fault names.
    */
    [[nodiscard]] result<void> check_values(string_type& where) const {
        for (std::size_t i = 0; i < values_.size(); ++i) {
            const std::size_t b = index(values_[i].base);
            if (!owns(i) && b >= i) {
                return fault(1, "values", "a value inside no value of the document before it");
            }
            if (!holds(values_[i].pointer)) {
                return fault(1, "values", "a value's pointer is not a pointer of the document");
            }
            if (addresses_[i] == nullptr) {
                where.clear();
                write_pointer(values_[i].pointer, where);
                return fault(11, where, "a value's pointer resolves to nothing");
            }
        }
        return {};
    }

    /**
     Whether `uri` is absolute and has no fragment: a scheme (RFC 3986 section 3.1), a colon, and
     no `#`.
    */
    static bool is_absolute_without_fragment(std::string_view uri) noexcept {
        const auto alpha = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); };
        const std::size_t colon = uri.find(':');
        if (colon == 0 || colon == std::string_view::npos || !alpha(uri.front()) ||
            uri.find('#') != std::string_view::npos) {
            return false;
        }
        return std::all_of(
            uri.begin() + 1, uri.begin() + static_cast<std::ptrdiff_t>(colon), [&](char c) {
                return alpha(c) || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
            });
    }

    /** Invariants 1 and 9 for the resource at index `i`. */
    [[nodiscard]] result<void> check_resource(std::size_t i) const {
        const resource& r = resources_[i];
        if (!holds(r.uri) || !holds(r.root) || index(r.parent) >= resources_.size()) {
            return fault(1, "resources", "a resource names a part the document does not hold");
        }
        const std::string_view uri = string(r.uri);
        if (!is_absolute_without_fragment(uri)) {
            return fault(9, uri, "a resource's URI is not absolute, or has a fragment");
        }
        const node& root = nodes_[index(r.root)];
        if (index(root.resource) != i) {
            return fault(9, uri, "a resource's root belongs to another resource");
        }
        if (root.pointer != empty_pointer) {
            return fault(9, uri, "a resource's root has a pointer other than \"\"");
        }
        return {};
    }

    /** Invariant 9: each resource is well formed, and no two share a URI. */
    [[nodiscard]] result<void> check_resources(scratch_type& scratch) const {
        scratch.clear();
        for (std::size_t i = 0; i < resources_.size(); ++i) {
            if (auto r = check_resource(i); !r) {
                return r;
            }
            scratch.push_back(static_cast<std::uint32_t>(resources_[i].uri));
        }
        if (const auto twice = duplicate(scratch, std::less<>{})) {
            return fault(9, string(string_id{*twice}), "two resources have one URI");
        }
        return {};
    }

    /** Invariant 10: each anchor names a node of its resource, and no name is given twice. */
    [[nodiscard]] result<void> check_anchors(scratch_type& scratch) const {
        scratch.clear();
        for (std::uint32_t i = 0; i < anchors_.size(); ++i) {
            const anchor& a = anchors_[i];
            if (index(a.resource) >= resources_.size() || !holds(a.name) || !holds(a.target)) {
                return fault(1, "anchors", "an anchor names a part the document does not hold");
            }
            if (nodes_[index(a.target)].resource != a.resource) {
                return fault(10, string(a.name), "an anchor's target is in another resource");
            }
            scratch.push_back(i);
        }
        const auto twice = duplicate(scratch, [&](std::uint32_t x, std::uint32_t y) {
            const anchor& a = anchors_[x];
            const anchor& b = anchors_[y];
            return std::tuple(a.resource, a.name, a.dynamic) <
                   std::tuple(b.resource, b.name, b.dynamic);
        });
        if (twice) {
            return fault(10, string(anchors_[*twice].name),
                         "an anchor's name is given twice in its resource");
        }
        return {};
    }

    allocator_type allocator_;
    node_id root_ = no_node;
    bool redefined_ = false;
    bool defined_unreserved_ = false;
    table<node> nodes_;
    table<bool> defined_;
    table<keyword> keywords_;
    table<node_id> node_lists_;
    table<named_subschema> entries_;
    table<string_id> string_lists_;
    table<dependency> dependencies_;
    table<value_id> value_lists_;
    table<ir::number<number_type>> numbers_;
    table<resource> resources_;
    table<anchor> anchors_;
    // The steps of every JSON Pointer, each after the step it extends.
    table<pointer_step> steps_;

    // The interned strings: a deque never moves its elements, so a view of one stays valid.
    boost::container::deque<string_type, rebind<string_type>> strings_;
    // A cache of views into strings_, ordered, so that interning never hashes untrusted text.
    boost::container::map<std::string_view, string_id, std::less<>, rebind<string_index_entry>>
        string_index_;

    // The values' storage, declared before the values so that it dies after them.
    std::shared_ptr<detail::value_arena<allocator_type>> arena_;
    // The values the document owns, each copied once into arena_.
    boost::container::deque<boost::json::value, rebind<boost::json::value>> owned_;
    // Each value id: the owned value it is in, and its pointer there.
    table<value_entry> values_;
    // A cache: the address each value id resolves to, or nullptr; rebuilt by a copy.
    table<const boost::json::value*> addresses_;
    // Scratch: the steps of the pointer being resolved, from the last.
    table<pointer_id> chain_;
};

/** The document of the default traits. */
using document = basic_document<default_traits>;

}  // namespace webcpp::json_schema::ir

#endif  // WEBCPP_JSON_SCHEMA_IR_DOCUMENT_HPP
