---
title: "webcpp CI inputs"
tags: [webcpp, ci, github-actions]
status: active
created: 2026-10-07
---

# webcpp CI inputs

Pinned values for the CI implementer to copy, not invent. Every value below
was read with `gh api` or downloaded and hashed; the "How verified" column
says which. Downloads went only to
`~/.claude/jobs/fcf3a902/tmp/ci-inputs/`. No write call was made
to any API.

## 1. Third-party GitHub Actions

xstate-cpp (`<private>/xstate-cpp/.github/workflows/ci.yml`,
`.github/actions/boost/action.yml`, `.github/actions/wasi-sdk/action.yml`)
pins four actions today, each by full commit SHA with the tag in a trailing
comment. `gh api repos/<owner>/<repo>/git/ref/tags/<tag>` returned a
lightweight tag (object type `commit`) for every one below, so no
dereference through `git/tags/<sha>` was needed.

| Action | xstate-cpp's pin today | Latest tag (2026-10-07) | Latest commit SHA | Conflict |
| --- | --- | --- | --- | --- |
| `actions/checkout` | `v7.0.1` @ `3d3c42e5aac5ba805825da76410c181273ba90b1` | `v7.0.1` | `3d3c42e5aac5ba805825da76410c181273ba90b1` | none, up to date |
| `actions/cache` (`/restore`, `/save`) | `v6.1.0` @ `55cc8345863c7cc4c66a329aec7e433d2d1c52a9` | `v6.1.0` | `55cc8345863c7cc4c66a329aec7e433d2d1c52a9` | none, up to date |
| `actions/setup-node` | `v7.0.0` @ `820762786026740c76f36085b0efc47a31fe5020` | `v7.0.0` | `820762786026740c76f36085b0efc47a31fe5020` | none, up to date |
| `actions/upload-artifact` | `v7.0.1` @ `043fb46d1a93c77aae656e7c1c64a875d1fc6a0a` | `v7.0.2` | `cf430e030ddbb5b0abf93d22962f4752f3646cd9` | **xstate-cpp is one patch behind.** webcpp's CI implementer should decide whether to pin the same `v7.0.1` xstate-cpp uses (for a byte-identical port) or move to `v7.0.2` |

Not used by xstate-cpp today, needed by webcpp for the superproject's Pages
job and the reusable library workflow's artifact hand-off (`doc/DESIGN.md`
section 6, stage1 plan Task 11):

| Action | Latest tag (2026-10-07) | Latest commit SHA | How verified |
| --- | --- | --- | --- |
| `actions/download-artifact` | `v8.0.2` | `9000827ccba6bdab643e8b6fd33ac0654aef8333` | `gh api repos/actions/download-artifact/releases/latest` then `git/ref/tags/v8.0.2` |
| `actions/configure-pages` | `v6.0.0` | `45bfe0192ca1faeb007ade9deae92b16b8254a0d` | same pattern |
| `actions/upload-pages-artifact` | `v5.0.0` | `fc324d3547104276b827a68afc52ff2a11cc49c9` | same pattern |
| `actions/deploy-pages` | `v5.0.1` | `368f82528645a54fb793d4d04e342629a3f51346` | same pattern |

All eight: tag resolved with `gh api repos/<owner>/<repo>/releases/latest
--jq '.tag_name'`, then the commit SHA with `gh api
repos/<owner>/<repo>/git/ref/tags/<tag>`. Every `object.type` returned was
`commit` (a lightweight tag), so none needed the annotated-tag dereference
through `git/tags/<sha>`.

## 2. Boost 1.92.0 source

Boost publishes a `.json` sidecar with a GitHub-computed SHA-256 next to
every archive at `archives.boost.io/release/1.92.0/source/`. The `.tar.bz2`
one was downloaded and re-hashed locally; it matched exactly.

| Asset | URL | SHA-256 | How verified |
| --- | --- | --- | --- |
| `boost_1_92_0.tar.bz2` | `https://archives.boost.io/release/1.92.0/source/boost_1_92_0.tar.bz2` | `5c1d40cb8e19adbf740a4ec2da35b3e58f3f5804b1dce44deb53df72193cbc6c` | Downloaded (199,030,664 bytes) and hashed with `shasum -a 256`; matches the published `.tar.bz2.json`. This is the asset Task 11 names for the Boost action |
| `boost_1_92_0.zip` | `https://archives.boost.io/release/1.92.0/source/boost_1_92_0.zip` | `c46baee75ed4b558cf6553630d0615a147ed582812917b57e6f0839d1954549f` | Read from the published `.zip.json` only (not re-downloaded: 315 MB). Needed for the Windows lane if the action extracts a zip there instead of running `.tar.bz2` through `tar` |

Both `.json` sidecars record the same source commit,
`afdfa32505af73e3d208144b3f623f0096cb62b6`.

Alternative source, not recommended: `boostorg/boost`'s GitHub release
`boost-1.92.0` ships different artifacts (`boost-1.92.0-b2-nodocs.tar.gz`,
73,484,649 bytes, SHA-256
`e2a814b3a158ab482c7a3d330f8bf5a7a8423d258a4e2fb396996e00fcee2111`, plus a
`cmake` variant) - these exclude the documentation tree that
`archives.boost.io`'s full source carries, so they are not a drop-in
replacement for Task 11's Boost action.

## 3. wasi-sdk 34

`gh api repos/WebAssembly/wasi-sdk/releases/tags/wasi-sdk-34`, digest field
per asset. The `x86_64-linux` SHA-256 matches what xstate-cpp's
`.github/actions/wasi-sdk/action.yml` already pins
(`b761e3a0721dbae9c09a0059e5fdb2bf917d1b4a8a7b430fb3b5aafb0984b2c4`), so the
existing action's value is confirmed current.

| Platform | Asset | SHA-256 | Size |
| --- | --- | --- | --- |
| Linux x86_64 | `wasi-sdk-34.0-x86_64-linux.tar.gz` | `b761e3a0721dbae9c09a0059e5fdb2bf917d1b4a8a7b430fb3b5aafb0984b2c4` | 192,383,077 |
| Linux arm64 | `wasi-sdk-34.0-arm64-linux.tar.gz` | `f7e243dff54d60bcc576e94d6166b69f410f2500ae4a9ceef34315be10e77971` | 192,255,247 |
| macOS arm64 | `wasi-sdk-34.0-arm64-macos.tar.gz` | `9c59398106b417f8f14913380fdf0097a8cc0ff4af9eb3ce0065a859e88d49e9` | 180,322,002 |
| macOS x86_64 | `wasi-sdk-34.0-x86_64-macos.tar.gz` | `87d27fa8adc68dee59bfbf2e22a6d34ef717c34d6bf1d8af2a56fc929d9ce0eb` | 182,668,326 |
| Windows x86_64 | `wasi-sdk-34.0-x86_64-windows.tar.gz` | `cccb5c323a9b34f0349a9b09e8804a0a7632c68c3310f4b5f437ed57d7e71d8f` | 619,003,408 |
| Windows arm64 | `wasi-sdk-34.0-arm64-windows.tar.gz` | `45e1c71f3e965621e7b98ebe1d37b0e4b1f77f3e8072113ffb4534e67b1a4b7c` | 620,307,231 |

Release tag: `wasi-sdk-34` (the asset version string inside the archive is
`34.0`); download base URL
`https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-34/`.

## 4. wasmtime 47.0.3

`gh api repos/bytecodealliance/wasmtime/releases/tags/v47.0.3`. The
`x86_64-linux` SHA-256 matches xstate-cpp's `ci.yml` pin
(`ca1fc56d1afc40c8782e96c297fd182a0da162f9a8f52a1e7b094e1dd648e178`),
confirming it is current.

| Platform | Asset | SHA-256 | Size |
| --- | --- | --- | --- |
| Linux x86_64 | `wasmtime-v47.0.3-x86_64-linux.tar.xz` | `ca1fc56d1afc40c8782e96c297fd182a0da162f9a8f52a1e7b094e1dd648e178` | 11,712,804 |
| Linux arm64 | `wasmtime-v47.0.3-aarch64-linux.tar.xz` | `497b518db00ae585f04390758eaa99ad555bee50612dce7d102602778fb46ff0` | 10,926,212 |
| macOS arm64 | `wasmtime-v47.0.3-aarch64-macos.tar.xz` | `c2684249e5d9ef9351942cf2d315982cf201fe0300f05d63bc1527446f0cd37f` | 9,830,600 |
| macOS x86_64 | `wasmtime-v47.0.3-x86_64-macos.tar.xz` | `424a50f76a9dcf4d02dab326b2374be1ad404030576ee915866e4af106058b35` | 11,197,160 |
| Windows x86_64 | `wasmtime-v47.0.3-x86_64-windows.zip` | `80ddf037820b35a9a53c13519632f52947e848d6ba69a483840b7330110408f3` | 13,283,825 |
| Windows arm64 | `wasmtime-v47.0.3-aarch64-windows.zip` | `be35951403bcf6cbf44043b78db4bcdb9918df7e53bd66f6aef3e651cc882b28` | 12,224,285 |

Download base URL:
`https://github.com/bytecodealliance/wasmtime/releases/download/v47.0.3/`.
(`-c-api` variants exist per platform too, for embedding the C API; not
needed here since the CI only runs `wasmtime serve` / `wasmtime` as a CLI.)

## 5. MrDocs 2026.9.29

`gh api repos/cppalliance/mrdocs/releases/tags/2026.9.29`, digest field. The
Linux asset was downloaded and re-hashed; it matches the digest and matches
the value already recorded in
`RESEARCH/MRDOCS_FOR_WEBCPP.md`.

| Asset | SHA-256 | Size | How verified |
| --- | --- | --- | --- |
| `MrDocs-2026.9.29-Linux.tar.xz` | `9d96e42f303046fb0b221aa9071eabccc40b9683e0b7af2b02a929d15476ca6a` | 44,759,560 | Downloaded and hashed with `shasum -a 256`; matches GitHub's digest field exactly |

Extraction directory: the downloaded archive's top-level entries are
`MrDocs-0.8.0-Linux/...` (confirmed with `tar -tJf`), the same pattern the
research note already measured for the Darwin archive
(`MrDocs-0.8.0-Darwin/`). A pin script must use the version-number directory
name (`0.8.0`), not the calendar tag (`2026.9.29`).

Download URL:
`https://github.com/cppalliance/mrdocs/releases/download/2026.9.29/MrDocs-2026.9.29-Linux.tar.xz`.

No checksum or signature is published by the MrDocs project itself (confirmed
again here); the SHA-256 above is GitHub's own per-asset digest, as the
research note already flagged. No macOS x86_64 or Linux arm64 build exists at
this release.

## 6. Node

The local machine runs Node 26.7.0. `gh api repos/nodejs/node/releases` and
`https://nodejs.org/dist/index.json` both confirm `v26.7.0` is a real,
published release, and that the 26.x line is **not** an LTS line yet
(`lts: false` for every 26.x entry as of 2026-10-07; 26 is an even major, so
it is expected to flip to LTS around late October 2026, after this date).

`actions/setup-node`'s own README states the `node-version` input accepts an
exact version string (its own examples use things like `10.15.1`), resolved
against the published Node distribution, not a bundled list restricted to
LTS lines. There is nothing that blocks pinning `node-version: 26.7.0`
directly - it is a real release and the action will download it like any
other exact version.

xstate-cpp pins `node-version: 24` in both its `compat` and `docs` jobs
(`actions/setup-node@...`, `ci.yml` lines 157, 226) - the current LTS line,
not an exact patch. webcpp's CI implementer should decide whether to follow
xstate-cpp's pattern (an LTS major, e.g. `24`, for stability across patch
releases) or pin the exact local version `26.7.0` for byte-identical
reproduction of what was developed against. Given Node 26 is not yet LTS,
following xstate-cpp's existing convention (pin an LTS major) argues for
staying on `24` rather than moving to `26.7.0`; this is a decision for the
owner, not a fact to invent.

## 7. GitHub-hosted runner images and compilers

xstate-cpp's `ci.yml` already runs green on every one of these (section 1 of
that file's own header comment, and the job matrix). No lane runs an
explicit `apt-get install` or similar step to add a compiler: every compiler
below is present on the stock image, confirmed against
`actions/runner-images`' own published software lists where a mismatch
seemed possible.

| Lane | Runner label | Image doc | Compiler confirmed preinstalled |
| --- | --- | --- | --- |
| GCC 14 | `ubuntu-24.04` | `images/ubuntu/Ubuntu2404-Readme.md` | Clang 16/17/18 listed explicitly (so `clang-18` needs no install); the readme's software table shows only the default `gcc` meta-package (13.2.0), but xstate-cpp's `ci.yml` runs `toolset=gcc-14` with no install step and is green, so `gcc-14` is present via Ubuntu 24.04's own apt repos as an alternative package, same as `clang-18` |
| GCC 15 | `ubuntu-26.04` | `images/ubuntu/Ubuntu2604-Readme.md` | Clang 20/21/22 listed explicitly (`clang-22` needs no install); default `gcc` meta-package is 15.2.0, and `toolset=gcc-15` runs with no install step in `ci.yml` |
| Clang 18 | `ubuntu-24.04` | same as above | Listed explicitly: `Clang: 16.0.6, 17.0.6, 18.1.3` |
| Clang 22 | `ubuntu-26.04` | same as above | Listed explicitly: `Clang: 20.1.8, 21.1.8, 22.1.2` |
| Apple Clang | `macos-15` | `images/macos/macos-15-Readme.md` | Xcode's bundled Clang; used by `ci.yml`'s `posix` matrix with `toolset: clang`, no install step |
| MSVC 14.3 (VS 2022) | `windows-2022` | `images/windows/Windows2022-Readme.md` | Visual Studio Enterprise 2022 (toolset v143) |
| MSVC 14.5 (VS 2026) | `windows-2025` | `images/windows/Windows2025-VS2026-Readme.md` | **Important**: the runner-images repo's own `README.md` table says the labels `windows-latest`, `windows-2025`, and `windows-2025-vs2026` all now resolve to this VS-2026 image, confirmed by its content showing "Visual Studio Enterprise 2026 18.10.12217.157". A separate, stale file `Windows2025-Readme.md` still exists in the repo and shows VS 2022 instead - it is not linked from the current README's table and should not be used as the reference; `Windows2025-VS2026-Readme.md` is the live doc for the `windows-2025` label today |

No conflict with xstate-cpp's current matrix: `ubuntu-24.04` /
`ubuntu-26.04` / `macos-15` / `windows-2022` / `windows-2025` are exactly the
five OS labels its `ci.yml` already uses.

## 8. GitHub Pages deployment (organisation project repo)

Not called; documented only, per the task's instruction.

- **Permissions**, on the job that deploys:
  ```yaml
  permissions:
    pages: write      # to deploy to Pages
    id-token: write   # to verify the deployment originates from an appropriate source
  ```
  (from `actions/deploy-pages`'s own README, read via `gh api
  repos/actions/deploy-pages/contents/README.md`).
- **Environment name:** `github-pages` - the README explicitly recommends
  against renaming it ("you may use a different environment name if needed,
  but this is not recommended").
- **Enabling Pages with `build_type=workflow`** (one-time, by the repository
  or org owner, not by CI): the stage1 plan's Task 12 already names the exact
  call:
  ```sh
  gh api -X POST repos/webcpporg/webcpp/pages -f build_type=workflow
  ```
  This is a write call and was not executed in this research task.

## Could not verify / open items

- **`actions/upload-artifact` version choice**: xstate-cpp pins `v7.0.1`;
  `v7.0.2` is now latest. This is the one real conflict between what
  xstate-cpp pins today and the current latest release (see section 1);
  it is a decision for the CI implementer/owner, not something this research
  resolves.
- **Node version choice**: whether to pin `24` (xstate-cpp's current
  convention, LTS) or the local `26.7.0` (not yet LTS) is likewise a decision
  left open (section 6); both are mechanically supported by
  `actions/setup-node@v7.0.0`.
- **wasi-sdk and wasmtime arm64/Windows SHA-256 values** were read from
  GitHub's per-asset `digest` field only (not downloaded and re-hashed,
  unlike the `x86_64-linux` ones and Boost's `.tar.bz2`), since the task
  scoped local downloads to what was needed to cross-check existing pins and
  the Boost/MrDocs values that had no existing pin to compare against.
