## Context

The organization's genesis (`changes/archive/2026-10-07-genesis/`) fixed one identity for every
commit of a maintainer and of the maintainer's agents, and the specification
(`specs/organization/spec.md`, "Superproject: Commit identity and form") and the superproject's
`AGENTS.md`, chapter 11, carry it with its address and the command that sets it. External
contributors already commit under their own name and address. The other parts of the rule, the
form of a commit, are what the lint and the reviews rely on.

## Goals / Non-Goals

**Goals:**

- One rule for every contributor: commit under your own git identity.
- Keep every rule about the form of a commit and about personal data in files.
- Remove the fixed address from the rules a contributor reads and runs.

**Non-Goals:**

- Rewriting history: no commit is amended, rebased or re-signed, and no archived proposal or
  recorded patch is edited.
- Changing `meta/libraries.json`'s `authors` and `maintainers` (open question O2).
- Requiring signed commits (open question O3).

## Decisions

### D1. Each contributor commits under their own identity

A contributor, human or agent, commits under a name and an e-mail address they control; a GitHub
noreply address (`<id>+<login>@users.noreply.github.com`) is allowed and is the suggested choice
for anyone who does not want to publish an address. An agent commits under the identity of the
account it acts for or of its own account, never under another person's.

Rejected: keeping one fixed identity for the maintainer and the maintainer's agents. It cannot be
followed by a second maintainer or by an agent of another account without impersonation, and it
keeps a personal address in the rules.

Rejected: a shared organization identity (such as `webcpp <noreply@webcpp.org>`). It hides who made
a change, which the review and the history need, and the organization has no mailbox behind it.

### D2. The form of a commit is unchanged

No trailer of any kind, explicit paths, a plain subject prefixed by the part it touches, no last
paragraph that starts with `word:`, English, no em dash. These are about the history's
readability and are independent of whose name is on a commit.

### D3. History and recorded material are not rewritten

Existing commits keep their author and committer. Archived proposals, the organization's genesis
reference and the patches recorded under `reference/` of open proposals keep the identity they
were made with: they are records of what was done, and changing them would make them disagree with
the commits they describe. Plans of open proposals that spell the old command are read under the
new rule once it is accepted; each is updated when its own proposal is next revised.

Rejected: a sweep that edits every occurrence of the old address in the repository. It would
rewrite records, touch proposals this change does not own, and gain nothing a reader of the rule
needs.

### D4. No tool checks the author's address

The superproject's tools check no author address (the lint checks only the banned word in each
commit's author, committer and message, and the tests commit under their own throwaway
identities), so the change is to the rules alone.

## Risks / Trade-offs

- [A contributor commits with a machine's default identity, such as an employer's address] ->
  `AGENTS.md` tells them to pass their identity explicitly, and the reviewer checks the author of
  a pull request's commits.
- [Agents of one account commit under different identities] -> the rule names the account an agent
  acts for; the owner may later fix one identity per agent account in that account's own
  configuration, outside the repositories.

## Migration Plan

1. Accept this change and merge the superproject's `AGENTS.md` pull request.
2. From then on, every new commit follows the new rule; nothing else is migrated.
3. On archive, the requirement's new text replaces the old one in `specs/organization/spec.md`.

## Open Questions

- **O1.** Should the old address stay mentioned anywhere in the current rules, for instance as the
  identity of the existing history? This proposal says no: the history shows it, the rules need
  not.
- **O2.** `meta/libraries.json` names `authors` `["R"]` and a maintainer address in every library,
  and the specification requires it. Should the maintainer field keep a personal address, take a
  noreply or organization address, or name a GitHub login? A separate change, since it touches
  every library.
- **O3.** Should commits to webcpporg repositories be signed (`git commit -S`, or GitHub's
  vigilant mode)? Not required by this change.
- **O4.** Should the CI check that a pull request's commits carry no trailer, which today only the
  reviewer checks?
