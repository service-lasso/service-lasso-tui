# Development instructions

## Branch boundary

- Development work starts from `develop` and uses an issue-scoped branch.
- Do not inspect, fetch, compare, branch from, merge from, or target `main`.
- `main` is reserved for an explicitly authorised release-promotion or urgent-hotfix role.

## Delivery workflow

1. Create or use an issue before implementation.
2. Make focused changes on an issue-scoped branch.
3. Run the relevant formatting, tests, and build checks.
4. Commit and push every completed change.
5. Open a pull request against `develop`; do not release or deploy from development work.

## Product expectations

The terminal UI is keyboard-first and consumes Service Lasso's runtime HTTP APIs.
It must make unavailable or failed API state clear, never treat a displayed
screen as evidence of a successful operation, and retain an accessible error
path for every operator action.
