# CLI/TUI ↔ Core connectivity review

Status: review only. This does not declare GA or change runtime behavior.

## Scope

Reviewed the current `service-lasso` Core source, the TUI API client/startup
path, and the external `service-lasso-cli` client. The question was not whether
each client has tests, but whether the three programs actually speak the same
HTTP contracts and entry lifecycle.

## Connectivity matrix

| Surface | Core `develop` | TUI | External CLI |
|---|---|---|---|
| Health / services | `/api/health`, `/api/services` | v1 compatibility path | Yes |
| Runtime identity / capabilities | `/api/runtime/instance`, `/api/runtime/capabilities` | v1 compatibility path | Yes |
| Setup / dependencies / inbox | `/api/setup/status`, `/api/dependencies`, `/api/operator/inbox` | v1 compatibility path | Setup yes; dependencies yes; inbox no |
| Durable lifecycle | `/api/operator/lifecycle/...` v1 | v1 compatibility only | Yes |
| Operator v2 discovery/forms/readback | **No `/api/operator/v2` route in Core source** | **Default path** | Not used |
| Reconciliation context | `/api/operator/lifecycle/reconciliation-context` | v2 client requests `/api/operator/v2/reconciliation-context` | Not used |
| Service registration | `/api/runtime/actions/importService`, `/api/operator/operations/:id` | Not used | Yes |

## Critical findings

### 1. TUI default API is not served by this Core

The TUI defaults to `ResolveOperatorConnections`, and its operator client prefixes
requests with `/api/operator/v2`
([operator_client.go:80](../internal/api/operator_client.go)).
The Core `develop` server has v1 lifecycle routes and no `/api/operator/v2`
route. A default TUI launch against this Core will fail before the first
successful read unless `--compat-v1` is used.

This is the highest-priority connectivity defect. Either Core must implement
the v2 surface, or the TUI default must move to the v1 surface.

### 2. CLI and TUI use different lifecycle contracts

The CLI uses `/api/operator/lifecycle/...` with confirmation id/phrase and
idempotency keys. The TUI v2 path uses discovery, reconciliation context,
service detail, lifecycle availability, and typed operation readback. They are
not two clients for one contract; they are two different operator models.

### 3. CLI exposes two lifecycle mutation paths

`service start|stop|restart` posts directly to `/api/services/{id}/{action}`
with `confirm: true`, while `operator execute` uses the durable lifecycle
contract. The direct path is a separate mutation surface and can bypass the
durable confirmation/readback semantics used by the operator workflow.

### 4. Authentication and connection configuration are split

- TUI: profile JSON with `url`, `tokenEnv`, `authMode`, `scopes`; local-admin or
  OAuth bearer.
- CLI: `coreUrl`/named origins plus environment-only
  `SERVICE_LASSO_CORE_TOKEN` and `SERVICE_LASSO_CLI_LOCAL_ADMIN_TOKEN`.

Both enforce token transport, but they do not share profiles, scope admission,
or credential semantics. An operator cannot configure one client and expect the
other to use the same connection.

### 5. Core entry boundary is not connected to the released TUI

The TUI has a Core-bound entry under the `service_lasso_core_entry` build tag
and a synthetic E2E path, but the release build still builds the default manual
entry. The Core producer/bundle config exists, but no CLI or release workflow
invokes it. The production producer also still maps bundle bytes into sections
rather than constructing real native originals.

### 6. Test coverage proves pieces, not the cross-product

Covered:
- Core v1 routes and CLI v1 client behavior.
- TUI v1 compatibility client.
- TUI v2 client against fixtures.
- Core entry synthetic bundle → producer → consumer → native contract.
- CLI/TUI unit tests.

Not covered:
- Default TUI v2 against this Core `develop`.
- CLI and TUI against the same live Core with the same profile.
- Core entry with an authorization bundle.
- Released TUI binary using the Core-bound entry.

## What is currently compatible

- CLI v1 reads and durable lifecycle requests match Core v1 routes.
- TUI `--compat-v1` uses the legacy client that matches Core v1 routes.
- Both clients reject token-on-cleartext transport for non-loopback origins.
- Core entry source contracts, producers, and synthetic E2E are consistent on
  Linux.

## Recommended order

1. **Pick one canonical HTTP contract.** Either implement `/api/operator/v2` in
   Core or make the TUI default to the v1 contract.
2. **Remove or explicitly isolate the CLI direct lifecycle mutation path.** Keep
   only the durable lifecycle surface unless the direct path is an approved
   compatibility exception.
3. **Unify connection/auth configuration** or document the two models as
   intentionally separate and test both against the same Core profile.
4. **Wire the Core entry producer/bundle into the actual production caller and
   release build**; keep the manual TUI entry as the explicit fallback.
5. **Add a cross-client acceptance test**: one Core instance, CLI v1, TUI v1
   compatibility, TUI default, and the Core entry path with the same identity and
   profile.
6. **Keep the release status `NOT_GA`** until the canonical contract and
   authorization inputs are decided.

## Evidence files

- Core routes: `service-lasso/src/server/index.ts`
- TUI default v2: `service-lasso-tui/cmd/service-lasso-tui/startup.go`,
  `service-lasso-tui/internal/api/operator_client.go`
- TUI v1 compatibility: `service-lasso-tui/internal/api/client.go`
- CLI v1: `service-lasso-cli/src/core-client.ts`
- Core entry: `service-lasso-tui/internal/coreentry/`,
  `service-lasso/src/runtime/entry/candidate-entry-production-config.ts`
