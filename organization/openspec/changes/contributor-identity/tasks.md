# Tasks

## 0. The proposal

- [x] 0.1 Write the proposal, the design, the tasks and the modified requirement; validate with `openspec validate --all --strict` and `tools/proposals.py check`
- [ ] 0.2 Settle O1 to O4 of `design.md` with the owner and record the answers in `design.md`
- [ ] 0.3 Submit the proposal for a decision (status `proposed`)

## 1. The superproject

- [x] 1.1 `AGENTS.md`, chapter 11 "Identity": every contributor commits under their own git identity, a noreply address allowed; the command passes the contributor's identity; no trailer, explicit paths, unchanged
- [x] 1.2 Check that no tool, lint rule or workflow checks the author's address (`git grep` of the old address and of `user.email` in `tools/` and `.github/`)
- [ ] 1.3 The superproject's pull request reviewed and merged by the owner

## 2. After the decision

- [ ] 2.1 Each open proposal's plan that spells the old command is updated when that proposal is next revised (design D3)
- [ ] 2.2 Archive the change, which writes the modified requirement into `specs/organization/spec.md`
