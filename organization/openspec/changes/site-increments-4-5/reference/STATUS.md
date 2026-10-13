# The site, increments 4 and 5: where the implementation stands, and how to resume

Written 2026-10-13, when this proposal took the plan in. `plan.md` is the plan (revision 3,
2026-10-11), `site-spec.md` the site specification (approved 2026-10-10) whose sections 6, 7, 8, 10
and 11 bind it, `../tasks.md` the checklist.

## Where it stands

**None of the plan's own tasks (Tasks 20 to 35) is built**, so the status is `accepted`:

- The published site (`webcpporg/webcpporg.github.io` at `7a4c03f`) is increment 1: it has no
  `app/`, `examples/`, `machines/`, `include/`, `tools/sizes.mjs` or `packages/`.
- The superproject's published `main` (`c53f3f9`) has none of the plan's tooling: no
  `webcpp.js-test`, no `webcpp.serve-native`, no `{webcpp-site}` attribute, no `lane-directories`.
- trystero's published tree (`160f431`) has no `raw_payload`, no `class app` (the shared
  connection is not integrated) and no `js/`.
- The plan's ledger (`WEBCPP_SITE_PLAN_4_5`, Task 20 Step 1) does not exist.

**The plan's preconditions are partly met, on a local branch that is not published.** The
superproject's branch `site` (a worktree on the maintainer's machine) holds 8 commits not in the
published history (`reference/superproject-patches/`, made with `git format-patch` from the merge
base with `main`): `7ee6310` "libs/xstate: on emscripten", `2916fbd` "the site's wasm, built with
emscripten by b2 site and the site's own lane", `a5005ae` and `e4d85d6` (a refused event is a value
and leaves the snapshot where it was), `f0090d2` "b2 site: the PWA's precache list, written last",
`0d0a003` and `6118793` and `de8470d` (the update failure, the trace and the player's sound, the
trace silent until the loader listens). Its submodule `libs/xstate` holds one unpublished commit,
`03796b1` "xstate on emscripten: the machine layer's tests and examples in the browser's target"
(`reference/xstate-patches/`), and the site's own repository seven commits above `9d3b9b1`
(`df15129` to `f3a05e6`, `reference/site-patches/`): the site's wasm running `machines/player.json`
and `install.json`, the PWA (manifest, service worker, install flow, precache list) and the trace.
These are the earlier site plan's Tasks 12 to 15, the preconditions "Site Tasks 12 to 18" of this
plan's table; its Tasks 16 to 19 (the visualizer, the calculator, the headless Chrome lane, the
review and publication) are not built. The work is a local branch, unpublished, and its ledger
records the maintainer stopped it on 2026-10-11 to review each task one by one.

Other preconditions: trystero's shared connection (`app`) is not in trystero's `main`;
webcpp::asio is not a submodule; `/webcpp//boost_url` is in the superproject's `main`, so R9 takes
its first branch.

### What was verified for this proposal

- From the repositories (read only), 2026-10-13: the absence of every file the plan creates in the
  published site, superproject and trystero trees (`grep` and `ls`), the absence of the plan's ledger
  directory, the eight superproject commits, the one xstate commit and the seven site commits
  (`git cherry`, `git format-patch`), and that the patches carry none of the plan's new names
  (`js-test`, `serve-native`, `raw_payload`, `sizes.mjs`, `packages/trystero`, `lane-directories`,
  `answer.hpp`).
- No build, test or lint was run for this proposal.

## Open items

1. **The owner's gates.** G4 (trystero's JavaScript API), G6 (npm publication), G7 (the public
   network from the site) and O5 (the sizes) are unresolved; the plan carries a recommendation for
   each (`design.md`, Open Questions). G5 and the controller's rulings Q1, Q2, O3 and O4 are settled.
2. **Preconditions.** The site's Tasks 16 to 19, the shared connection, and (for Task 33 only) the
   asio track; the plan says which tasks may start without each ("Startable now": Tasks 20 and 21,
   Task 27's size tool, Task 23 Steps 1 and 2 and Task 31 once asio's Tasks 2 and 3 are in `main`).
3. **The local branch is unpublished.** Its commits must be integrated, reviewed and pushed (gate G2)
   before Task 20's baseline is meaningful; the plan's Task 20 Step 4 rebases onto `main`.
4. **The `trystero.dev` page may change;** the provenance test holds the copy to the hash recorded on
   2026-10-11.

## How to resume

1. Read `plan.md` whole, then this file. Work in a superproject worktree on branch `site`
   (`git worktree add`), with the site and trystero as submodules, as the plan's "Repositories and
   paths" say; every b2 command runs from the superproject as
   `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam`,
   with `-a` for a verdict and its own `--build-dir` per lane.
2. Start with Task 20: the ledger, the baseline, each precondition checked and recorded, and the
   rebase onto the shared connection once it is in `main`. A precondition not met blocks only the
   tasks that need it.
3. Task 21 (the specification of trystero's JavaScript API) and Task 27's size tool (without
   `--check`) may start at once; Tasks 22 to 25 wait for G4.
4. Commit as `R <github.overboard312@passmail.net>` with no trailer, staging explicit paths; a
   library's commit is followed by a superproject commit of its pointer by path.
5. Before saying a task is done: its failing tests seen red then green, the lint, the tools' tests
   that it touches, the lanes from scratch, `b2 -a site`, the site's `npm test` and `typecheck` both
   with and without b2, and the browser lane; nothing is pushed or published without the owner.
