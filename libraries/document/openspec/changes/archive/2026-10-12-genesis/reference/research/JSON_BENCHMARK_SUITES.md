---
title: "JSON and dictionary benchmark suites: operations, datasets, methodology"
tags: [webcpp, document, benchmark, json, javascript, v8]
status: active
created: 2026-10-12
---

# JSON and dictionary benchmark suites

Research of 2026-10-12 for webcpp::document's benchmark table (Boost.JSON, webcpp::document,
JavaScript objects in V8 under Node.js and headless Chrome). Read or run on that date.

**Main finding:** no recognised JSON suite times DOM mutation (insert, update, delete a member,
array splice), deep copy or equality. JSON suites cover parse, stringify, prettify, traversal and
key-path queries; mutation, copy and iteration trace to hash-map suites, V8's js-perf-test and
Dromaeo's DOM-core tests.

## 1. JSON suites

- **nativejson-benchmark** (Milo Yip, https://github.com/miloyip/nativejson-benchmark, MIT): Parse,
  Stringify, Prettify, Statistics (a full DOM traversal counting objects, arrays, numbers, strings,
  true, false, null, memberCount, elementCount, stringLength; src/test.h), SaxRoundtrip,
  SaxStatistics, memory, allocations, code size; verification by Stat counts after a roundtrip.
  Data: canada.json, citm_catalog.json, twitter.json.
- **simdjson** (https://github.com/simdjson/simdjson/tree/master/benchmark): DOM API
  (serialize_twitter, prettify_twitter, twitter_count, twitter_default_profile, twitter_image_sizes,
  numbers_scan); task benchmarks with a Boost.JSON implementation (boostjson.h): find_tweet,
  top_tweet, distinct_user_id, partial_tweets, kostya, large_random, amazon_cellphones (parse
  included in each).
- **Boost.JSON bench/** (https://github.com/boostorg/json/tree/develop/bench, BSL-1.0): Parse and
  Serialize only (against RapidJSON, nlohmann), a null-parser variant, bench-conv.cpp
  (value_to/value_from); 14 files from simdjson-data.
- **serde json-benchmark** (https://github.com/serde-rs/json-benchmark): DOM and struct parse and
  stringify on the 3 nativejson files, MB/s.
- **nlohmann** (tests/benchmarks/src/benchmarks.cpp): ParseFile, ParseString, ParseIndented,
  ParseStringOrdered, Dump (compact, indent 4).
- **Glaze json_performance**, **daw_json_link**, **yyjson_benchmark**: struct-oriented or read/write
  only. **JetStream 2.1**: json-parse-inspector, json-stringify-inspector.

## 2. Dictionary and JavaScript object suites

- **Tessil hopscotch benchmark** (https://tessil.github.io/2016/08/29/benchmark-hopscotch-map.html):
  shuffled and full inserts, inserts with reserve, deletes, reads, read misses, reads after deleting
  half, iteration, memory; integer keys and 15- and 50-character string keys; best of 5.
- **martinus map_benchmark** (https://martin.ankerl.com/2022/08/27/hashmap-bench-01/): Copy, Insert
  then Erase, Random Insert & Access, Random Insert & Erase, Iterate, Find with hit rates; median.
- **abseil** raw_hash_set_benchmark.cc (Iteration, EraseEmplace, TransparentFind, CopyCtor,
  CopyAssign) and linked_hash_map_benchmark.cc (an insertion-ordered map, as a JS object:
  TryEmplace, InsertOrAssign, short and long keys, hit and miss, sizes 1 to 65536).
- **V8 js-perf-test** (https://github.com/v8/v8/tree/main/test/js-perf-test): Collections/map.js
  (Set, Get, Has, Delete, Iteration), Keys/keys.js (Object.keys, for-in), PropertyQueries (`in`,
  hasOwnProperty, fast vs dictionary mode), Object/ (assign, create, entries, values), Array/ (at,
  every, filter, find, forEach, map, reduce, slice, some, join; no push or splice).
- **V8 fast properties** (https://v8.dev/blog/fast-properties): adding and deleting many properties
  moves an object to dictionary mode; integer-like keys are elements.
- **Dromaeo DOM core** (WebKit PerformanceTests/Dromaeo): dom-modify (appendChild, insertBefore,
  removeChild, cloneNode), dom-traverse, dom-attr, dom-query.

## 3. Datasets

- canada.json (2,251,051 bytes): GeoJSON, number-heavy, 55,563 coordinate pairs, depth 7.
- citm_catalog.json (1,727,204): wide objects up to 184 keys, ~10.9k objects, 293 integer-like keys.
- twitter.json (631,514): string-heavy, CJK and escapes, depth 10; user content.
- simdjson-data (https://github.com/simdjson/simdjson-data): gsoc-2018, marine_ik, mesh, random,
  update-center, github_events, numbers, twitterescaped; "for educational and testing purposes".
- No dataset carries a clear data licence (the repositories are MIT or BSL-1.0, the data's provenance
  is not stated); twitter.json is the riskiest.

## 4. Methodology

- nativejson: 10 trials, minimum. Boost.JSON: 6 trials of at least 5 s after a warm-up, drop worst 2
  and best 1. serde: best of 2. simdjson: Google Benchmark. martinus: median, isolated core, fixed
  frequency. kostya/benchmarks (C++ vs Node): median plus or minus MAD over 10 runs, loading untimed.
  V8 base.js: warm-up then at least 1 s, geometric means. JetStream: first iteration, worst 4 and
  average separate cold JIT from steady state. mitata: do_not_optimize and gc after warm-up.
- Chrome clamps performance.now to 100 us unless crossOriginIsolated (COOP + COEP: 5 us); Node is not
  clamped.
- Fair practice: the same V8 major in Node and Chrome; COOP/COEP; --expose-gc and gc() between samples;
  samples of at least 10 ms; steady-state median of at least 10 samples plus the first-iteration time;
  a checksum sink in JS and benchmark::DoNotOptimize-style sinks in C++; verify every result (Stat
  counts).

## 5. Recommended operation set

| # | Operation | Sources | Data, metric | Notes |
| --- | --- | --- | --- | --- |
| 1 | Parse | nativejson, Boost bench, serde, nlohmann, JetStream | canada, citm, twitter; MB/s | |
| 2 | Stringify compact | nativejson, Boost, nlohmann, simdjson, JetStream | the same; MB/s | byte equality with JS for webcpp only |
| 3 | Prettify | nativejson, nlohmann, simdjson | the same; MB/s | Boost.JSON has no pretty printer in the library |
| 4 | Full traversal (Statistics) | nativejson | the same; ms, MB/s | checked against Stat counts |
| 5 | Key-path queries | simdjson find_tweet, distinct_user_id, top_tweet | twitter; per query, parse excluded | Boost: simdjson's boostjson.h |
| 6 | Lookup hit and miss | Tessil, martinus, V8 PropertyQueries, abseil | citm 184-key object; synthetic 10..10^5 members, 15/50-char keys; ns/op | |
| 7 | Insert a member | Tessil, abseil TryEmplace, V8 Map-Set | synthetic; ns/op | integer-like keys reorder in JS and webcpp |
| 8 | Update a member | abseil InsertOrAssign, martinus | citm, synthetic; ns/op | |
| 9 | Delete a member, then lookups | Tessil, martinus, V8 Map-Delete, Dromaeo removeChild | synthetic, citm; ns/op | Boost: stable_erase matches JS; JS delete enters dictionary mode |
| 10 | Iterate entries | Tessil, martinus, abseil, V8 Keys/Object, Dromaeo traverse | citm, synthetic; ns/member | |
| 11 | Array push and pop | Dromaeo appendChild/removeChild, martinus | twitter statuses, canada rings; ns/op | |
| 12 | Array splice in the middle | Dromaeo insertBefore/removeChild | canada coordinates; ns/op | Boost composes insert/erase |
| 13 | Array methods (map, filter, reduce, find, slice, join) | V8 js-perf-test Array | canada, twitter; ns/element | Boost and webcpp hand-written where no method exists |
| 14 | Deep copy | martinus Copy, abseil CopyCtor, Dromaeo cloneNode | canada, citm, twitter; MB/s | JS: structuredClone and JSON.parse(JSON.stringify) |
| 15 | Equality | none | | not traced: leave out or label "not traced" |
