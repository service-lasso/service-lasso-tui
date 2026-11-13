# Git workflow

Development starts from `develop` in an issue-scoped branch. Do not inspect,
fetch, compare, branch from, merge from, or target `main` during normal
development. Commit each intentional change, push it, and open a pull request
to `develop`. Hosted CI and independent review are required before any parent
role considers merge or release decisions.
