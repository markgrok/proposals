# Working on config

Read [webcpp's AGENTS.md](../../AGENTS.md)
(<https://github.com/webcpporg/webcpp/blob/main/AGENTS.md>) first; this file
holds only what is config's own.

- **Macros only.** config declares no namespace; its macros are the
  collection's, `WEBCPP_*`, as Boost.Config's are `BOOST_*`, and its include
  guards `WEBCPP_CONFIG_HPP` and `WEBCPP_VERSION_HPP`.
- **`<webcpp/version.hpp>` lies outside `webcpp/config/`,** as Boost's
  `<boost/version.hpp>`: headers-alone takes `config.hpp`, and the test
  `version` compiles `version.hpp` alone.
- **The version is written once,** in `version.hpp`, and moves only by the
  release procedure (AGENTS.md chapter 11), right after a tag.
- **The target comes from the compiler,** never from b2, so a plain compiler
  gets it too; the target block comes before Boost.Config, which a
  WebAssembly target without a C++ library cannot compile.
