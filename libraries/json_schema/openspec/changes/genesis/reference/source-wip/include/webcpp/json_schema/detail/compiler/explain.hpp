// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The text of a plan: each block, then each of its instructions with its operands in words and its
 keyword's absolute location.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_COMPILER_EXPLAIN_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_COMPILER_EXPLAIN_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/compiler/opcode.hpp>
#include <webcpp/json_schema/detail/compiler/plan.hpp>
#include <webcpp/json_schema/detail/json/pointer.hpp>
#include <webcpp/json_schema/ir/document.hpp>
#include <webcpp/json_schema/ir/ids.hpp>
#include <webcpp/json_schema/ir/keyword.hpp>
#include <webcpp/json_schema/ir/node.hpp>
#include <webcpp/json_schema/numbers/number.hpp>
#include <webcpp/json_schema/traits.hpp>

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <string>
#include <string_view>
#include <variant>

namespace webcpp::json_schema::detail {

/**
 Writes the text of a plan.

 A block is a line `block <n> <resource URI>#<node pointer>`; each of its instructions a line of
 two spaces, its index in the block, its opcode's name, its operands in words, then ` ; ` and its
 keyword's absolute location, for an instruction that has one. A pointer is written as a URI
 fragment, as an error's absolute keyword location is. A block's pointer is spelled once, for its
 header and every location in it.

 @tparam Traits The traits of the document and the plan.
*/
template <traits Traits>
class explainer {
public:
    /** The document's number type. */
    using number_type = Traits::number_type;

    /**
     A writer of the text of `compiled`, lowered from `document`, into `resource`.

     @param document The document; it must outlive the writer.
     @param compiled The plan; it must outlive the writer.
     @param resource The resource of the text and of the scratch.
    */
    explainer(const ir::basic_document<Traits>& document, const plan<Traits>& compiled,
              std::pmr::memory_resource& resource)
        : document_(document), plan_(compiled), out_(&resource), pointer_(&resource),
          location_(&resource) {}

    /**
     Writes the text.

     @return The text, every block in order.
    */
    std::pmr::string write() {
        for (std::size_t i = 0; i < plan_.blocks.size(); ++i) {
            const block& b = plan_.blocks[i];
            pointer_.clear();
            document_.write_pointer(document_.at(b.node).pointer, pointer_);
            out_.append("block ");
            append_number(i);
            out_.push_back(' ');
            append_uri(b.node);
            append_fragment(out_, pointer_);
            out_.push_back('\n');
            for (std::uint32_t j = 0; j < b.count; ++j) {
                write_instruction(b, j);
            }
        }
        return std::move(out_);
    }

private:
    /** Writes the line of the instruction `j` of the block `b`. */
    void write_instruction(const block& b, std::uint32_t j) {
        const instruction& in = plan_.code[b.first + j];
        out_.append("  ");
        append_number(j);
        out_.push_back(' ');
        out_.append(opcode_name(in.op));
        write_words(in);
        if (in.location == no_location) {
            out_.push_back('\n');
            return;
        }
        const keyword_location& at = plan_.locations[in.location];
        location_.clear();
        if (at.node == b.node) {
            location_.append(pointer_);
        } else {
            document_.write_pointer(document_.at(at.node).pointer, location_);
        }
        append_token(location_, document_.string(keyword_at(at).name));
        out_.append(" ; ");
        append_uri(at.node);
        append_fragment(out_, location_);
        out_.push_back('\n');
    }

    /** The keyword a location names. */
    [[nodiscard]] const ir::keyword& keyword_at(const keyword_location& at) const noexcept {
        const std::uint32_t first = document_.at(at.node).keywords.first;
        return document_.keywords(at.node)[static_cast<std::uint32_t>(at.keyword) - first];
    }

    /** Writes the resource URI of `node`, then `#`. */
    void append_uri(ir::node_id node) {
        const ir::resource& r =
            document_.resources()[static_cast<std::size_t>(document_.at(node).resource)];
        out_.append(document_.string(r.uri));
        out_.push_back('#');
    }

    /** Writes an unsigned integer in decimal. */
    void append_number(std::uint64_t n) {
        std::array<char, 24> digits{};
        const auto written = std::to_chars(digits.data(), digits.data() + digits.size(), n);
        out_.append(digits.data(), written.ptr);
    }

    /** Writes a space, then a block's index, or `none` for @ref no_block. */
    void append_block(std::uint32_t b) {
        out_.push_back(' ');
        if (b == no_block) {
            out_.append("none");
        } else {
            append_number(b);
        }
    }

    /** Writes a space, then a count held as its two words. */
    void append_count(std::uint32_t low, std::uint32_t high) {
        out_.push_back(' ');
        append_number((std::uint64_t{high} << 32U) | low);
    }

    /** Writes a space, then a number operand, as its shortest text. */
    void append_operand(const ir::number<number_type>& n) {
        out_.push_back(' ');
        if (const auto* i = std::get_if<std::int64_t>(&n)) {
            std::array<char, 24> digits{};
            const auto written = std::to_chars(digits.data(), digits.data() + digits.size(), *i);
            out_.append(digits.data(), written.ptr);
        } else if (const auto* u = std::get_if<std::uint64_t>(&n)) {
            append_number(*u);
        } else if (const auto* d = std::get_if<number_type>(&n)) {
            std::array<char, 64> text{};
            out_.append(text.data(), number_traits<number_type>::to_chars(text, *d));
        }
    }

    /** The operands' words, each after a space, by the opcode's family. */
    void write_words(const instruction& in) {
        switch (in.op) {
            case opcode::type_guard:
            case opcode::assert_type:
            case opcode::assert_const:
            case opcode::assert_enum:
            case opcode::assert_multiple_of:
            case opcode::assert_maximum:
            case opcode::assert_exclusive_maximum:
            case opcode::assert_minimum:
            case opcode::assert_exclusive_minimum: write_value_words(in); return;
            case opcode::assert_length:
            case opcode::assert_item_count:
            case opcode::assert_property_count:
            case opcode::assert_pattern:
            case opcode::assert_required:
            case opcode::assert_dependent_required: write_size_words(in); return;
            case opcode::all_of:
            case opcode::any_of:
            case opcode::one_of:
            case opcode::not_:
            case opcode::if_then_else: write_applicator_words(in); return;
            case opcode::loop_properties:
            case opcode::loop_property_names:
            case opcode::loop_dependent_schemas:
            case opcode::loop_items:
            case opcode::loop_contains: write_loop_words(in); return;
            default: return;
        }
    }

    /** Writes a type's names, joined by `|`, in the order of their bits. */
    void append_types(std::uint16_t bits) {
        constexpr std::array<std::string_view, 7> names{
            "null", "boolean", "object", "array", "number", "string", "integer",
        };
        char separator = ' ';
        for (std::size_t i = 0; i < names.size(); ++i) {
            if ((bits & (1U << i)) != 0) {
                out_.push_back(separator);
                out_.append(names[i]);
                separator = '|';
            }
        }
    }

    /** The words of a guard, `type`, `const` (none), `enum` (the count) and the numbers. */
    void write_value_words(const instruction& in) {
        switch (in.op) {
            case opcode::type_guard:
                append_types(in.small);
                append_count(in.a, 0);
                return;
            case opcode::assert_type: append_types(in.small); return;
            case opcode::assert_const: return;
            case opcode::assert_enum: append_count(in.b, 0); return;
            default: append_operand(document_.number(ir::number_id{in.a})); return;
        }
    }

    /**
     The words of a size's limit (`max <n>` or `min <n>`), a pattern, `required` (the names joined
     by `,`) and `dependentRequired` (the count).
    */
    void write_size_words(const instruction& in) {
        switch (in.op) {
            case opcode::assert_pattern:
                out_.push_back(' ');
                out_.append(document_.string(
                    std::get<ir::text>(keyword_at(plan_.locations[in.location]).data).value));
                return;
            case opcode::assert_required: {
                char separator = ' ';
                for (std::uint32_t i = 0; i < in.b; ++i) {
                    out_.push_back(separator);
                    out_.append(document_.string(ir::string_id{plan_.operands[in.a + i]}));
                    separator = ',';
                }
                return;
            }
            case opcode::assert_dependent_required: append_count(in.b, 0); return;
            default:
                out_.append((in.flags & flag_bit) != 0 ? " max" : " min");
                append_count(in.a, in.b);
                return;
        }
    }

    /** The words of `allOf`, `anyOf`, `oneOf` and `not` (the blocks) and `if` (the three). */
    void write_applicator_words(const instruction& in) {
        if (in.op == opcode::not_) {
            append_block(in.a);
            return;
        }
        if (in.op == opcode::if_then_else) {
            append_block(in.a);
            append_block(plan_.operands[in.b]);
            append_block(plan_.operands[in.b + 1]);
            return;
        }
        for (std::uint32_t i = 0; i < in.b; ++i) {
            append_block(plan_.operands[in.a + i]);
        }
    }

    /** The words of the five loops. */
    void write_loop_words(const instruction& in) {
        switch (in.op) {
            case opcode::loop_properties:
                out_.append(" names");
                append_count(plan_.operands[in.a], 0);
                out_.append(" patterns");
                append_count(plan_.operands[in.b + 1], 0);
                out_.append(" additional");
                append_block(plan_.operands[in.b]);
                return;
            case opcode::loop_dependent_schemas:
                for (std::uint32_t i = 0; i < in.b; ++i) {
                    append_block(plan_.operands[in.a + (2 * i) + 1]);
                }
                return;
            case opcode::loop_items:
                out_.append(" prefix");
                append_count(plan_.operands[in.a], 0);
                out_.append(" rest");
                append_block(in.b);
                return;
            case opcode::loop_contains:
                append_block(in.a);
                out_.append(" min");
                append_count(plan_.operands[in.b], plan_.operands[in.b + 1]);
                out_.append(" max");
                if ((in.flags & flag_bit) == 0) {
                    out_.append(" none");
                } else {
                    append_count(plan_.operands[in.b + 2], plan_.operands[in.b + 3]);
                }
                return;
            default: append_block(in.a); return;
        }
    }

    const ir::basic_document<Traits>& document_;
    const plan<Traits>& plan_;
    std::pmr::string out_;
    /** The current block's node pointer, spelled once. */
    std::pmr::string pointer_;
    /** An instruction's keyword pointer, spelled. */
    std::pmr::string location_;
};

/**
 The text of a plan, for a reader: what each block does, in words.

 @tparam Traits The traits of the document and the plan.
 @param document The document the plan was compiled from.
 @param compiled The plan.
 @param resource The resource of the text and of its scratch.
 @return The text: a line per block, then a line per instruction.
 @throws std::bad_alloc When the resource throws it; without exceptions, the program ends.
*/
template <traits Traits>
std::pmr::string explain(const ir::basic_document<Traits>& document, const plan<Traits>& compiled,
                         std::pmr::memory_resource& resource) {
    explainer<Traits> writer(document, compiled, resource);
    return writer.write();
}

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_COMPILER_EXPLAIN_HPP
