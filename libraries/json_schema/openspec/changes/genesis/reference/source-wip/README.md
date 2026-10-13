# Work in progress: task E1.10, the compiler

These files are the uncommitted work of task E1.10 (`plan.md`, "Task E1.10: The compiler: the
plan, its lowering, and its text"), as they stood on the library's branch `e1-compiler`, on top
of the snapshot in `../source/` (branch `e1`, commit `80c3b69`). Each file is whole; copy the
tree over a checkout of `../source/` to apply it.

## What each file holds

| File | State | What it holds |
| --- | --- | --- |
| `include/webcpp/json_schema/detail/compiler/opcode.hpp` | new, complete for E1 | `enum class opcode` with values 0 (`ret`) to 27 (`loop_contains`), each enumerator's Doc Comment saying how it reads `flags`, `small`, `a` and `b`; `opcode_name`, the names `explain` writes (`not` for `not_`) |
| `include/webcpp/json_schema/detail/compiler/plan.hpp` | new, complete for E1 | `no_block`, `no_location`, `flag_bit`; `instruction` (`op`, `flags`, `small`, `a`, `b`, `location`), `block` (`first`, `count`, `node`, `flags`), `keyword_location` (`keyword`, `node`); `plan<Traits>` with its tables `code`, `blocks`, `locations`, `operands`, `regexes` and `root_block`, every table a `std::vector` over the traits' allocator rebound (`std::pmr::vector` with the default traits) |
| `include/webcpp/json_schema/detail/compiler/compile.hpp` | new, complete for E1 | `lowering<Traits>`: blocks reserved on first reach and lowered in that order, so the block table is the breadth-first queue (no recursion); one helper per row of the plan's lowering table (`lower_type`, `lower_constants`, `lower_bound`, `lower_count`, `lower_unique`, `lower_pattern`, `lower_required`, `lower_list`, `lower_subschemas`, `lower_conditional`, `lower_properties`, `lower_items`, `lower_contains`); fused keywords lowered once where the first stands; `type_guard` before each one-type keyword; each pattern compiled once per distinct text through the traits' regex policy (an ordered map, never a hash), a refusal returned as `errc::invalid_regex` with `where` the keyword's pointer (plus the member's name for `patternProperties`) and `what` the pattern; `compile_plan(document, allocator, options)` |
| `include/webcpp/json_schema/detail/compiler/explain.hpp` | new, complete for E1 | `explainer<Traits>` and `explain(document, plan, resource)`: a header line per block (`block <n> <resource URI>#<pointer>`), then a line per instruction (index, opcode name, operands in words, ` ; ` and the keyword's absolute location) |
| `test/compiler_test.cpp` | new, the plan's test as written | three cases: assertions guarded by type, blocks in breadth-first order, a bad pattern refused by place |
| `test/Jamfile` | the snapshot's, with two lines appended | `webcpp.headers-alone json_schema : ../include : json_schema/detail/compiler/*.hpp ;` and `webcpp.run compiler : compiler_test.cpp ;` |
| `meta/include-boundaries.json` | the snapshot's, with one object added | `detail/compiler/*.hpp` must not include `boost/locale`, `webcpp/regex`, `srell`, `boost/url`, `webcpp/lexical` |

## How far it got

Measured on 2026-10-12 in a scratch copy of the superproject that still carried webcpp::regex
(the snapshot's dependency until task 1.4 is redone), from scratch (`b2 -a`):

- native (Apple clang 21): `b2 libs/json_schema/test` passes, 76 targets, the four
  headers-alone builds of `detail/compiler/` and `compiler` among them;
- wasip2 (wasi-sdk 34, wasmtime 47): `b2 toolset=clang-wasip2 testing.launcher=wasmtime
  libs/json_schema/test//compiler` passes.

Differences from the plan's sketch, each deliberate and to keep:

- `compile_plan` takes the compile options as a third argument (default `compile_options()`),
  since it reads `regex_step_budget`; the plan's sketch passed only the allocator.
- The plan's tables are `std::vector` over the traits' allocator, not `std::pmr::vector`, so a
  traits type with another allocator works; with the default traits they are the same type.
- `loop_properties`'s name table is sorted by the names' bytes, so the VM can find a name by binary search; the
  pattern table keeps source order.
- `next_index` refuses a table past 2^32 - 1 entries through `boost::throw_exception`
  (`std::length_error`), since 2^32 - 1 is `no_block` and `no_location`.

## What is missing before E1.10's commit

1. **Task 1.4 first.** The boundary object above names `webcpp/regex` and `srell`; after the
   redo it names `boost/regex` instead, as every other object of the file does then.
2. **The other lanes:** wasip3, Emscripten and gcc-15, and the whole test directory on wasip2,
   from scratch (`lanes libs/json_schema/test`, `plan.md` "Global Constraints").
3. **The lint** (`lint_clean`): clang-format and clang-tidy have not run on these files. Points
   to look at: `emit_guarded` and `lower_*` initialize `instruction` with designated
   initializers that skip members (the plan's clang-tidy rule wants every member of an aggregate
   of two or more named); the cognitive complexity of `lower_keyword` and of `explainer`'s
   `write_*` functions (limit 25); the `NOLINT` on `opcode::not_`.
4. **A review point:** a plan past 2^32 - 1 entries raises `std::length_error` (an abort without
   exceptions) where the library's rule is errors as values; either keep it as an allocation
   failure (the documented `bad_alloc` path) and say so in `compile_plan`'s Doc Comment, or
   return a new `errc`, appended. The plan does not settle it.
5. **Coverage of the lowering table:** the plan's test pins `type`, `maxLength`, `minLength`,
   `maximum`, `properties`, `required`, `allOf`, a boolean subschema and a refused pattern. The
   rows for `enum`, `const`, `multipleOf`, the exclusive bounds, item and property counts,
   `uniqueItems`, `dependentRequired`, `anyOf`, `oneOf`, `not`, `if`/`then`/`else`,
   `patternProperties`, `additionalProperties`, `propertyNames`, `dependentSchemas`,
   `prefixItems`/`items` and `contains` are written but no `explain` golden pins their words;
   E1.11's `vm_test` pins their verdicts. Adding a golden per row is cheap and recommended.
6. **The commit**, as the plan's Step 5 writes it, then E1.11.
