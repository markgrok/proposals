# Tasks

Every task of 0.1.0, each pointing to its task in `reference/plan.md` and to its commits in the
library's repository (`webcpporg/idna`, subjects after "idna: "); a checked task is done in
`reference/source/`. Nothing remains for 0.1.0.

## 1. The repository and the skeleton (plan: Tasks 0 and 1)

- [x] 1.1 The integration space, the library's repository and the pre-flight: baseline lint, the network check, and the ruling on the Unicode data notice (plan: Task 0; the controller's work, no commit in the library)
- [x] 1.2 The skeleton: `build.jam`, `meta/`, the CI file, `README.md`, `AGENTS.md`, the page's introduction, `config.hpp`, `errors.hpp`, `options.hpp` (plan: Task 1; `be969f4`)
- [x] 1.3 The errors name IdnaTestV2's codes, the V5 case and its option, and the output buffer's guard comes first (plan: Task 1; `2e7796e`)

## 2. Punycode and strict UTF-8 (plan: Task 2)

- [x] 2.1 `punycode::encode` and `decode`, span and pmr forms, RFC 3492 samples (A) to (S), and strict UTF-8 (`86f45cf`)
- [x] 2.2 Punycode checks its input before it writes, and the tests pin every boundary the review found loose (`b686973`)

## 3. The Unicode 17.0.0 tables (plan: Tasks 3 and 4)

- [x] 3.1 The generator, the tables and their lookups in `detail/properties.hpp` (plan: Task 3; `45d6c1c`)
- [x] 3.2 The generator stops on a property value it does not know, and the lookups state their domain (plan: Task 3; `a7aeb6e`)
- [x] 3.3 The page's Unicode section and the Unicode data notice (plan: Task 4; `a0bbc86`)
- [x] 3.4 The page's move to another Unicode version names every file it changes; the Unicode notes are one section (plan: Task 4; `be05752`)

## 4. The conformance vectors and the check of generated files (plan: Tasks 5 and 6)

- [x] 4.1 The vectors of IdnaTestV2.txt and NormalizationTest.txt, generated (plan: Task 5; `fff7a94`)
- [x] 4.2 The vectors test pins an explicit empty status, and the Bidi exemption covers its three rows alone (plan: Task 5; `921587c`)
- [x] 4.3 The own lane `tables`, which checks the generated files (plan: Task 6; `a1bcff6`)
- [x] 4.4 `AGENTS.md` writes the generated files with `update-generated` (plan: Task 6; `b327f7a`)

## 5. Normalization, contextual rules and the Bidi rule (plan: Tasks 7 and 8)

- [x] 5.1 NFC within a label's storage, inline or growing (plan: Task 7; `c5eeaf0`)
- [x] 5.2 NFC's timing test is relative to a baseline, and a long run's buffer is on the stack only while it is sorted (plan: Task 7; `c1f6fbd`)
- [x] 5.3 The contextual rules of RFC 5892 and the Bidi rule of RFC 5893 (plan: Task 8; `6ddca42`)
- [x] 5.4 The whole-label contextual rules in linear time, and an empty label in the Bidi rule (plan: Task 8; `15b9739`)
- [x] 5.5 The contextual rules' timing test is relative to a baseline, as NFC's is (plan: Task 8; `9a12e30`)

## 6. IDNA2008 validity (plan: Tasks 9 and 10)

- [x] 6.1 The label driver, and IDNA2008 validity of labels and domains (plan: Task 9; `30ea760`)
- [x] 6.2 The validators against IdnaTestV2 as given, and the review's minors (plan: Task 9; `3943ee3`)
- [x] 6.3 The validity page orders a label beyond the storage before its rules (plan: Task 9; `4543152`)
- [x] 6.4 The validity example, on the page (plan: Task 10; `1ddeced`)

## 7. UTS 46 ToASCII and ToUnicode (plan: Tasks 11 and 12)

- [x] 7.1 ToASCII and ToUnicode, conformant to IdnaTestV2 (plan: Task 11; `f4ffaaf`)
- [x] 7.2 The decoder's bound pinned without timing, and no timing test left to flake (plan: Task 11; `5beb705`)
- [x] 7.3 The conversions on the page, their examples, and the limits (plan: Task 12; `23a6883`)
- [x] 7.4 The page states which conversion meets which half of Punycode's bound (plan: Task 12; `406df57`)

## 8. Readiness (plan: Task 13)

- [x] 8.1 The page complete, its guarantees, the README, and the counts the page reads from the data (`4f5a666`)
- [x] 8.2 The guarantees say exactly what holds, and their tests cite them (`214be09`)
- [x] 8.3 The errors test pins `error_code`'s message into a buffer as `noexcept` (`f93c715`)
- [x] 8.4 The Doc Comments name `storage_exhausted` where it can come, and the page's error order and example tags are made exact (`a8024d1`)

## 9. Publication and alignment with the superproject (plan: Task 14)

- [x] 9.1 The whole-branch review, the lanes from scratch, the repository `webcpporg/idna` and its submodule `libs/idna` in the superproject (plan: Task 14; the controller's work, recorded in `reference/STATUS.md`)
- [x] 9.2 `config.hpp` includes `<webcpp/config.hpp>`, and `WEBCPP_NO_EXCEPTIONS` disables its exceptions (`c7f6db7`)
- [x] 9.3 The page's and the test report's addresses at the organisation's root (`9784d6d`)
- [x] 9.4 A changelog, the page's last appendix (`35528cc`)
- [x] 9.5 The Unicode directory's address on the page is text, not a link inside code (`48caf2a`)
- [x] 9.6 The vectors' strings are `sv` literals, within every compiler's constant-evaluation steps (`e2452e5`)
