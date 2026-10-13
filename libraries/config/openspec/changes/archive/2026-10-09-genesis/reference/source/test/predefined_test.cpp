// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// A WEBCPP_TARGET_* macro or WEBCPP_HAS_EXCEPTIONS defined before <webcpp/config.hpp>, which
// test/Jamfile defines on the command line, stops the build with config.hpp's message alone.

// expected-error@webcpp/config.hpp:* {{are defined by <webcpp/config.hpp> alone}}
#include <webcpp/config.hpp>
