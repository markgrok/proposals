# Tasks

Every task of 0.1.0, reconstructed from the library's commit history (there is no separate plan
document), each pointing to its commits in `reference/source/`'s repository; a checked task is done
in `reference/source/`. The commits are listed in the order they entered the repository, which
differs from their author dates in a few places.

## 1. 0.1.0: the library moves and is renamed

- [x] 1.1 The repository: headers renamed to `webcpp::xactor` under `include/webcpp/`, `build.jam`
  (`/webcpp/xactor//xactor`), the seven first examples with their `.expected` files, `README.md`,
  `AGENTS.md`, `LICENSE_1_0.txt`, `meta/libraries.json`, a headers-alone `test/Jamfile` (`284b68a`)
- [x] 1.2 Targets native, wasip2 and wasip3 declared for the examples, `xactor_asio` native only
  (`284b68a`)

## 2. 0.1.0: the tests on lightweight_test

- [x] 2.1 The four suites `scheduler`, `lifecycle`, `fuel` and `create_actor` converted from
  Boost.Test, with `test/require.hpp`, `test/.clang-tidy` and the equivalence record
  `test/CONVERSION.md` (55 cases, 295 assertions, three planted defects) (`5d4d77b`)
- [x] 2.2 A failing check in a loop prints the loop's item (`517f0e1`)

## 3. 0.1.0: Doc Comments and the strict reference

- [x] 3.1 Every public symbol has a Doc Comment; `doc/mrdocs.yml` gives each enumerator a section;
  MrDocs builds the reference strict (`edf900c`)
- [x] 3.2 The Doc Comments corrected after review: the drivers, the log, `spend`, the Asio driver
  (`7d6bebb`, `be91db1`)

## 4. 0.1.0: the documentation page

- [x] 4.1 The page `doc/xactor.adoc` and its sections, the glossary, the 36 numbered guarantees and
  the eighth example `xactor_testing` (`b30e860`)
- [x] 4.2 The page corrected from review: what the code keeps, the testing chapter, the introduction
  naming xstate (`31ce42e`, `497e7e6`, `d50d064`, `b99d152`)

## 5. 0.1.0: continuous integration

- [x] 5.1 The superproject's reusable workflow for xactor, and a run on `main` never cancelled
  (`c001c01`, `59775a1`)

## 6. 0.1.0: exceptions are the program's choice

- [x] 6.1 Exceptions off on wasip2 only, then every target built with exceptions and the headers
  compiled without them (`88cbbdb`, `992eaed`)
- [x] 6.2 `config.hpp` and `WEBCPP_XACTOR_NO_EXCEPTIONS`, with the `config`,
  `config_developer_defined` and `config_all_off` compile tests (`1d41ce8`)
- [x] 6.3 `config.hpp` includes `<webcpp/config.hpp>`; `WEBCPP_NO_EXCEPTIONS` disables xactor's
  exceptions with every library's (`1b1b159`, `37092ab`)

## 7. 0.1.0: changelog and release

- [x] 7.1 The changelog appendix, its lines saying what a user sees (`f5de1c2`, `4457dae`)
- [x] 7.2 Small corrections: the testing chapter's `BOOST_TEST` call, the test report's address
  (`4358e03`, `8f28b25`)
- [x] 7.3 The maintainer's public address in `meta/libraries.json`; the tree at `62f3e77` is release
  0.1.0 (`62f3e77`)
