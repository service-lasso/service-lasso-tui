# Lasso TUI: a terminal Service Admin for Service Lasso

> **Status:** proposed next design objective, recorded 2026-11-25. Not yet mapped to a GitHub issue, an active spec section, or the capability matrix. Nothing here is implemented or validated against a running Core.
>
> **Relationship to this repository.** This design was drafted from a read-only review of `lasso-serviceadmin` and `service-lasso`. The implementation contract below has been reconciled with this repository:
> - The stack is **Go + Bubble Tea + Lip Gloss** per `docs/framework-decision.md` and the existing `cmd/` / `internal/` layout. The TypeScript/Ink proposal is rejected.
> - Lifecycle actions use operator/v2 discovery, availability, preview, explicit confirmation, execute, and durable readback. Direct Admin lifecycle routes are not a TUI source of truth.
> - Admin-only routes that are not verified against current Core are Core dependencies, not client fallbacks.
> - Section 5.1 separates verified routes from dependencies/out-of-scope work.
>
> The parts intended for this repository are the Service Admin screen map (Section 5), layout and mockups (Sections 3–4), keymap (Section 6), live-update, error and theming guidance (Sections 7–10), the phased scope (Section 12) and the gap list (Section 13).

Design for a keyboard-first terminal UI that mirrors **lasso-serviceadmin** and controls a local **Service Lasso** runtime over the same HTTP API that Admin uses.

Prepared 25 Nov 2026 (AEDT). Read-only review of `D:\projects\service-lasso\lasso-serviceadmin` (working tree on develop) and `D:\projects\service-lasso\service-lasso` (Core API taken from **`origin/develop`**, because the local checkout is on the stale branch `release/reconcile-develop-1801`). Nothing was edited, built or run.

---

## 1. What I found

### 1.1 Admin (lasso-serviceadmin)
- Package `@service-lasso/service-admin` 1.0.0-rc.1. React 19, Vite 8, TanStack Router + React Query + Table, Zustand, shadcn/Radix, Tailwind 4, cmdk command menu, Monaco (config editor), @xterm/xterm (terminal attach), @xyflow/react (dependency graph), recharts.
- Packaged as service `@serviceadmin` (UI port **17700**, runs `node runtime/server.js`, depends on `@node`). `runtime/server.js` proxies `/api` to Core at `SERVICE_LASSO_API_BASE_URL` = `http://127.0.0.1:17883` and stamps trusted-proxy headers.
- Dev: `VITE_SERVICE_LASSO_API_BASE_URL`, optional stub mode `VITE_SERVICE_LASSO_ENABLE_STUB_DATA=true`.
- Live updates are **polling only** (React Query `refetchInterval: 30000` on dashboard/metrics/instance/network; terminal output polled every **200 ms**).
- Terminology: Service, Runtime, Dashboard, Favorites, Problem services, Generation lane, Inbox, Recovery, Updates, Setup steps, Endpoints, Routes (Traefik), Installed, Variables (Global/Service, Plain/Secret), Network exposure (Local/LAN/Public), Security (Groups, Permission catalogue, Actor assignments, Provider mappings), MCP (Clients, Confirmations, Operations, Role matrix), Secrets Broker (RAM files, Secrets, Providers, Topology, Review).
- Service status values: `running | available | stopped | degraded`; health: `healthy | warning | critical`.

### 1.2 Live navigation (sidebar-data.ts)
| Group | Item | Route |
|---|---|---|
| Service Admin | Dashboard | `/` |
| | Services | `/services` (+ detail `/services/$serviceId`) |
| | Dependencies | `/dependencies` |
| | Routes | `/service-routes` |
| | Logs | `/logs` |
| | Runtime | `/runtime` |
| | MCP | `/mcp` |
| | Installed | `/installed` |
| | Variables | `/variables` |
| | Network | `/network` |
| | Security | `/security` |
| Operations | Inbox | `/inbox` |
| | Telemetry | `/operations/telemetry` |
| | Audit | `/operations/audit-logging` |
| Secrets Broker | RAM files | `/secrets-broker/webdav` |
| | Secrets | `/secrets-broker/secrets` |
| | Providers | `/secrets-broker/sources` |
| | Topology | `/secrets-broker/topology` |
| | Review | `/secrets-broker/review` |
| Configuration | Settings › Appearance | `/settings/appearance` |
| | Help Center | `/help-center` |

Also routed but not in the sidebar: `/settings/startup` (start enabled services automatically), auth screens (local-operator sign-in, first-run setup), error pages 401/403/404/500/503, and several `/secrets-broker/*` sub-pages. **Excluded from the TUI:** `chats`, `users`, `tasks`, `apps` (shadcn template leftovers), Policy Simulation and Sessions (retired).

Service detail tabs (`service-detail-tabs.ts`): Overview (Ctrl+1), Dependencies, Endpoints, Variables, Config, Logs, Terminal, Setup, Access, Secrets (Ctrl+0).

Services table columns: Favorite, Service, Status, Links, Updates, Recovery, Controls, Open.

### 1.3 Core API the TUI will use (origin/develop)
Default port: `SERVICE_LASSO_PORT ?? 18080` (`src/index.ts`); packaged/demo stack uses **17883**. All JSON.

**Discovery / identity**
- `GET /api/health` → `{ service: "service-lasso", status: "ok", api: { status, version } }`
- `GET /api/runtime`, `GET /api/runtime/instance`, `GET /api/runtime/capabilities`, `GET /api/runtime/doctor`
- `GET /api/runtime/security` (identity: actor, auth posture)
- `POST /api/runtime/auth/local` body `{ method: "token", token }` or `{ method: "password", username, password }` → `{ session: { token } }`
- `GET /api/runtime/auth/first-run`, `POST /api/runtime/auth/first-run/acknowledge`
- Instance files: `<workspaceRoot>/.service-lasso/runtime-instance.json` (has `apiUrl`, `apiPort`, `pid`, `heartbeatAt`) and `~/.service-lasso/instances.json` (registry).

**Dashboard / services**
- `GET /api/dashboard` → `{ summary }` (counts, favorites, others, problemServices, warnings, update/recovery notifications)
- `GET /api/dashboard/services` → `{ services: DashboardService[] }`
- `GET /api/dashboard/services/:id` → `{ service }`
- `GET /api/metrics` (fleet: crashCount, lastTermination, stdout/stderr line counts)
- `PATCH /api/services/:id/meta` `{ favorite }` · `GET /api/services/meta`

**Lifecycle**
- Legacy/Admin direct lifecycle (`POST /api/services/:id/:action`) is **not used by the TUI**. The TUI uses operator/v2 availability, preview, confirmation, execute, and readback.
- `POST /api/runtime/actions/startAll | stopAll | reload` (Admin's "restart all" = stopAll then startAll). Core also has `shutdown`, `importService`, `importService/plan`.
- `GET /api/services/:id/start-trace` (phases: dependency_resolution → port_selection → artifact_acquisition → env_merge → process_spawn → health_check → terminal_outcome)

**Health / recovery / updates / setup**
- `GET /api/services/:id/health`, `/health/history`, `/recovery`, `/recovery/restart-preflight`; `POST /api/services/:id/recovery/doctor`; `GET /api/recovery`
- `GET /api/updates`, `POST /api/updates/check`, `GET /api/services/:id/updates`, `POST /api/services/:id/update/download | install` `{ force }`, `GET .../update/install/plan`
- `GET /api/services/:id/setup`, `POST /api/services/:id/setup/run[/:stepId]`

**Config / variables / network / routes**
- `GET|PUT /api/services/:id/config` (PUT `{ content, actor, reason }`), `GET .../config/backups`, `GET .../config-drift`
- `GET /api/variables`, `GET /api/services/:id/variables`, `GET /api/globalenv`
- `GET /api/network`, `GET /api/services/:id/network`, `GET /api/dependencies`, `GET /api/services/:id/dependents`

**Logs and terminal**
- `GET /api/services/log-info?service=&type=` (path, availableTypes, stdin capability)
- `GET /api/services/:id/logs` (overview + recent entries)
- `GET /api/logs/read?service=&type=&source=&limit=&before=` → `{ lines, entries, totalLines, hasMore, nextBefore }` (backward paging only; **no `after`**)
- `GET /api/logs/search?service=&type=&q=&cursor=&limit=&includeArchives=`
- `POST /api/services/:id/stdin` `{ input, stream: "stdin", actor }`
- `GET /api/services/:id/terminal`; `POST .../terminal/attach` → `{ sessionToken, readOnly, rows, cols, expiresAt }`; `GET .../terminal/output?after=<seq>` (base64 `data`); `POST .../terminal/input` `{ data: base64 }`; `POST .../terminal/resize` `{ rows, cols }`. Header `x-service-lasso-terminal-token`.

**Operations**
- Inbox: `GET /api/operator/inbox?filter=&limit=&cursor=`, `GET .../counts`, `POST .../:id/read | hide | unhide`, `POST .../bulk { action, ids }`
- `GET /api/telemetry`, `GET /api/services/:id/telemetry`
- `GET /api/audit?serviceId=&actor=&action=&outcome=&source=&since=&until=&query=&limit=&cursor=`
- Operator queue: `/api/operator/actions`, `/api/operator/confirmations/:id/confirm | execute`, `/api/operator/lifecycle/operations`
- `GET /api/mcp`, `GET /api/mcp/info` (Core also has an MCP stdio adapter)
- `GET /api/security`, `GET|PUT /api/runtime/settings/startup`
- Catalog: `GET /api/catalog`, `/api/catalog/packages`, `POST /api/catalog/install`
- Secrets Broker: proxied via `/api/services/%40secretsbroker/proxy/v1/...` (kv, management/secrets/*/dry-run|apply, policy/preview|apply) plus `/api/secrets/*` rotation/audit routes.

**Auth model** (`request-policy.ts`)
- Loopback clients get **implicit `local-root`** unless they present a credential.
- Credential: `x-service-lasso-admin-token: <token>` or `Authorization: Bearer <token>`; env `SERVICE_LASSO_LOCAL_ADMIN_TOKEN` or the local-operator vault token. Session TTL 12 h.
- Remote clients may be forced to SSO (`FORCE_SSO`); trusted-proxy headers are only honoured from Admin's loopback proxy, so the TUI must **not** send them.

**Core CLI** (`src/cli.ts`): `serve | start | stop | restart [--port --port-policy]`, `install <id>`, `setup list|run`, `updates list|check|download|install`, `plan start|stop|autostart|update-install|import`, `recovery status|doctor|restart-preflight`, `health history`, `instance`, `doctor status`, `readiness gate`, `lockfile`, `config-drift`, `config-apply preflight`, `config-snapshot export|import`, `secrets …`, `backup …`, `diagnostics bundle`, `operator actions …`, `services import`, `template check-upgrade`, `release verify-manifest`. Most take `--json`.

---

## 2. Stack

**Go + Bubble Tea + Lip Gloss**, matching `docs/framework-decision.md` and the existing TUI modules.

Why:
- It is the recorded repository decision and keeps one Go toolchain for CLI/TUI-adjacent delivery.
- Single static binary, no Node runtime for the TUI, and the existing terminal ownership work
  (`coreentry`, `lifetime`, PTY/ConPTY probes) is reusable.
- Current modules already split client, model, layout, and entry ownership; parity work should
  extend them rather than introduce a second renderer.
- Bubble Tea provides the alternate-screen, raw-mode, resize, and Windows terminal behavior the
  design needs.

Rejected alternatives:
- **TypeScript + Ink**: violates the recorded framework decision and introduces a second runtime
  and type surface.
- **Rust + Ratatui**: fastest, but a third language and no reuse of current terminal ownership code.
- **TS + OpenTUI**: least mature Windows path and not aligned with this repository.

The screen map, layout, keymap, and polling rules are renderer-neutral and remain the target.

## 3. Layout

```
┌─ Lasso TUI ────────────────────────────────────────────────────────────────────────────────────┐
│ SIDEBAR (22 cols)   │ MAIN PANE (list or detail)                                                │
│ Service Admin       │                                                                           │
│ Operations          │                                                                           │
│ Secrets Broker      │                                                                           │
│ Configuration       ├───────────────────────────────────────────────────────────────────────────┤
│                     │ LOG DOCK (toggle with `), follows the selected service                    │
├─────────────────────┴───────────────────────────────────────────────────────────────────────────┤
│ STATUS BAR: connection · actor · runtime health · counts · inbox unread · last refresh · hints  │
└─────────────────────────────────────────────────────────────────────────────────────────────────┘
```
- Sidebar mirrors `sidebar-data.ts` groups; collapses to icons/initials under 100 cols, hidden under 80.
- Main pane: a table screen or a detail screen with tabs.
- Log dock: bottom split showing the selected service's tail; `` ` `` toggles, `+`/`-` resizes.
- Status bar: always visible; turns red/amber when offline or degraded.
- Overlays: command palette, confirm dialogs, toasts, help.

---

## 4. Mockups (120×34)
Service names other than `@serviceadmin`, `@node` and `@secretsbroker`, and all numbers, are illustrative.

### 4.1 Dashboard
```
┌ Lasso ─────────────────────────────────────────────────────────────────────────── ● healthy  127.0.0.1:17883 ┐
│ SERVICE ADMIN       │ Dashboard                                                     generation lane: selected│
│ ▸ Dashboard         │ ┌ Runtime health ┐┌ Services ──────┐┌ Inbox unread ┐┌ Listen ports ┐┌ Log volume ┐ │
│   Services          │ │ ● healthy      ││ 12 total       ││ 3            ││ 14           ││ 18.2k lines│ │
│   Dependencies      │ │ reloaded 09:41 ││ 9 run · 1 deg  ││              ││ Traefik: 6   ││ 2 crashes  │ │
│   Routes            │ └────────────────┘│ 2 stopped      │└──────────────┘└──────────────┘└────────────┘ │
│   Logs              │                   └────────────────┘                                             │
│   Runtime           │ Warnings and problem services                                                    │
│   MCP               │  ▲ @secretsbroker   degraded   health check failing for 2m      [enter] open     │
│   Installed         │  ■ @example-api     stopped    exited with code 1               [s] start        │
│   Variables         │                                                                                  │
│   Network           │ Favorite services                                                                │
│   Security          │  ★ @serviceadmin    ● running  healthy  3h 12m   ui http://127.0.0.1:17700/      │
│ OPERATIONS          │  ★ @node            ● running  healthy  3h 12m                                   │
│   Inbox         3   │                                                                                  │
│   Telemetry         │ All other services (8)                                         [/] filter        │
│   Audit             │  ● @traefik  ● @postgres  ○ @mailpit  ● @worker  …                               │
│ SECRETS BROKER      │                                                                                  │
│   RAM files         │ Runtime actions:  [A] start all   [Z] stop all   [⇧R] restart all   [⇧L] reload  │
│   Secrets           │                                                                                  │
├─────────────────────┴──────────────────────────────────────────────────────────────────────────────────┤
│ ● connected local-root │ 9▶ 1▲ 2■ │ inbox 3 │ refreshed 2s ago │ ^K palette  ? help  q quit              │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Services list
```
│ Services                    12 total · 9 running · 1 degraded · 2 stopped     filter: █              │
│ ★  SERVICE           STATUS       HEALTH    UPTIME   UPDATES     RECOVERY   LINKS                     │
│ ★  @serviceadmin     ● running    healthy   3h 12m   latest      ok         ui                        │
│ ★  @node             ● running    healthy   3h 12m   latest      ok                                   │
│ ▸  @secretsbroker    ▲ degraded   warning   3h 11m   available   attention  api                       │
│    @traefik          ● running    healthy   3h 12m   latest      ok         dashboard                 │
│    @example-api      ■ stopped    critical  —        latest      blocked                              │
│    @mailpit          ○ available  —         —        —           —          ui                        │
│ ─────────────────────────────────────────────────────────────────────────────────────────────────────│
│ enter open  s start  S stop  r restart  L reload  i install  f fav  o open url  y copy url  ` logs   │
```

### 4.3 Service detail (Overview tab)
```
│ @secretsbroker  ▲ degraded · warning                                       ← esc back               │
│ 1 Overview  2 Dependencies  3 Endpoints  4 Variables  5 Config  6 Logs  7 Terminal  8 Setup  9 Access 0 Secrets │
│ ┌ Run status ──────────────────────┐ ┌ Isolation ────────────────────┐ ┌ Updates ─────────────────┐ │
│ │ state     running                │ │ mode     direct               │ │ installed 1.0.0-rc.1     │ │
│ │ health    warning                │ │ require  none                 │ │ available 1.0.0-rc.2     │ │
│ │ pid       18244   run r_8f2…     │ │ limits   not enforced         │ │ [d] download [I] install │ │
│ │ uptime    3h 11m                 │ └───────────────────────────────┘ └──────────────────────────┘ │
│ │ last check 09:52:10              │ ┌ Recovery ─────────────────────────────────────────────────┐ │
│ │ summary   health check failing   │ │ monitor attention · last restart 07:40 · [D] run doctor   │ │
│ └──────────────────────────────────┘ └───────────────────────────────────────────────────────────┘ │
│ Start trace (last attempt)                                                                         │
│  ✓ dependency_resolution  ✓ port_selection  ✓ artifact_acquisition  ✓ env_merge                   │
│  ✓ process_spawn          ✗ health_check: http 503 from /health        ○ terminal_outcome          │
│ Recent logs ───────────────────────────────────────────────────────────────────────────────────── │
│ 09:52:08 stderr  provider vault unreachable, retrying in 5s                                       │
│ s start  S stop  r restart  L reload  c config  f fav  o open                                     │
```

### 4.4 Logs (full screen or dock)
```
│ Logs  @secretsbroker   type: [stdout] stderr access error   ● following       lines 18,204           │
│ 09:51:58  INFO  listening on 127.0.0.1:17890                                                         │
│ 09:52:03  WARN  provider vault slow (1200 ms)                                                        │
│ 09:52:08  ERROR provider vault unreachable, retrying in 5s                                           │
│ 09:52:13  INFO  provider vault reconnected                                                           │
│ ▌                                                                                                    │
│ ─────────────────────────────────────────────────────────────────────────────────────────────────────│
│ f follow  t type  / search  n/N next/prev  PgUp older  w wrap  > send stdin  e export  esc back      │
```

### 4.5 Command palette (Ctrl+K or :)
```
                ┌ > rest█ ────────────────────────────────────────────┐
                │ SERVICES                                            │
                │ ▸ @restapi                         open details     │
                │ ACTIONS                                             │
                │   Restart @secretsbroker           r                │
                │   Restart all services             ⇧R               │
                │ NAVIGATE                                            │
                │   Runtime                          g u              │
                │ THEME                                               │
                │   Dark · Light · System                             │
                └─────────────────────────────────────────────────────┘
```

### 4.6 Confirm dialog
```
                ┌ Stop all services? ─────────────────────────────────┐
                │ POST /api/runtime/actions/stopAll                   │
                │ 9 running services will be stopped in dependency    │
                │ order. @serviceadmin will go offline.               │
                │                                                     │
                │            [ y ] Stop all      [ N ] Cancel         │
                └─────────────────────────────────────────────────────┘
```

### 4.7 Offline / not running
```
┌ Lasso ───────────────────────────────────────────────────────────────── ○ offline ┐
│                                                                                   │
│   Service Lasso runtime is not reachable.                                         │
│                                                                                   │
│   Tried:  --api flag                         not set                              │
│           SERVICE_LASSO_API_BASE_URL         not set                              │
│           .service-lasso/runtime-instance.json  stale (heartbeat 14m ago)         │
│           http://127.0.0.1:17883/api/health  connection refused                   │
│           http://127.0.0.1:18080/api/health  connection refused                   │
│                                                                                   │
│   Retrying in 4s …                                                                │
│                                                                                   │
│   [r] retry now   [s] start runtime (service-lasso start)   [u] enter URL  [q] quit│
└───────────────────────────────────────────────────────────────────────────────────┘
```

### 4.8 Terminal attach (full-screen passthrough)
```
┌ @worker · terminal attached (rw) · 120×32 ───────────────────── Ctrl+] detach ┐
│ <the service's own TUI or shell, rendered byte-for-byte>                       │
└────────────────────────────────────────────────────────────────────────────────┘
```

---

## 5. Screen map (Admin → TUI)

| Admin screen | TUI screen | Data (Core endpoint) | Actions in TUI | Phase |
|---|---|---|---|---|
| Dashboard `/` | Dashboard | `/api/dashboard`, `/api/metrics`, `/api/runtime/instance`, `/api/network`, `/api/operator/inbox/counts` | start all, stop all, restart all, reload, open problem service | MVP |
| Services `/services` | Services table | `/api/dashboard/services` | start/stop/restart/reload/install/config, favorite, open/copy URL | MVP |
| Service detail · Overview | Detail › 1 | `/api/dashboard/services/:id`, `/start-trace`, `/health` | lifecycle, recovery doctor, updates | MVP |
| · Dependencies | Detail › 2 | service `dependencies`/`dependents` | jump to service | MVP |
| · Endpoints | Detail › 3 | service `endpoints`, `links` | open/copy URL | MVP |
| · Variables | Detail › 4 | `/api/services/:id/variables` | copy (secrets masked) | MVP (read) |
| · Logs | Detail › 6 | `/api/services/log-info`, `/api/logs/read`, `/api/logs/search` | follow, type, search, page older | MVP |
| · Config | Detail › 5 | `GET/PUT /api/services/:id/config`, `/config/backups`, `/config-drift` | edit in `$EDITOR`, diff, save with reason | P2 |
| · Terminal | Detail › 7 | `/terminal`, `/attach`, `/output`, `/input`, `/resize`, `/stdin` | attach, detach, send line | P2 |
| · Setup | Detail › 8 | `/api/services/:id/setup` | run all / run step / force | P2 |
| · Access | Detail › 9 | service `access` | read | P3 |
| · Secrets | Detail › 0 | broker proxy | read-only refs | P3 |
| Dependencies | Dependencies | `/api/dependencies` | tree view, jump | P2 |
| Routes | Routes | services + `/api/network` | open/copy | P2 |
| Logs | Logs (service picker) | as above | as above | MVP |
| Runtime | Runtime | `/api/dashboard/services` | lifecycle per row | MVP |
| MCP | MCP | `/api/mcp`, `/api/mcp/info`, `/api/operator/confirmations` | copy stdio cmd / HTTP URL, confirm/execute | P3 |
| Installed | Installed | services `metadata` | copy paths | P2 |
| Variables | Variables | `/api/variables`, `/api/globalenv` | filter, copy | P2 |
| Network | Network | `/api/network` | open/copy | P2 |
| Security | Security | `/api/security` | read | P3 |
| Inbox | Inbox | `/api/operator/inbox*` | read/hide/unhide/bulk | P2 |
| Telemetry | Telemetry | `/api/telemetry`, `/api/services/:id/telemetry` | read | P3 |
| Audit | Audit | `/api/audit` | filter, page | P2 |
| Broker: RAM files, Secrets, Providers, Topology, Review | Broker screens | `/api/services/%40secretsbroker/proxy/v1/...`, `/api/secrets/*` | read-only first; no reveal | P3 |
| Settings › Appearance | Settings | local config file | theme, density, icons | MVP |
| Settings › Startup | Settings | `/api/runtime/settings/startup` | toggle autostart | P2 |
| Help Center | Help (`?`) | bundled keymap + doc links | — | MVP |
| Sign-in / first run | Login / first-run | `/api/runtime/auth/local`, `/first-run` | token or password | MVP |
| Add service (catalog / archive) | Add service | `/api/catalog/packages`, `/api/catalog/install` | install from catalog | P3 |

---

### 5.1 Endpoint status

**Verified TUI runtime path (operator/v2):** discovery, services, detail, reconciliation context,
lifecycle availability, preview, execute, operation readback, cancel, graph, variables, endpoints,
health history, history, logs, inbox, queue, audit, and service metadata. These are the only routes
used for TUI reads and mutations today. Several return valid empty pages until Core exposes richer
data; empty is not evidence of Admin parity.

**Missing or not advertised:** config/setup/declared-action/update/removal forms, populated graph,
and true running-operation cancellation. These require Core work or explicit product decisions
before they can appear in the TUI.

**Admin-only candidates, not yet verified for this TUI:** `/api/dashboard*`, `/api/metrics`,
`/api/network`, `/api/dependencies`, `/api/catalog*`, `/api/mcp*`, `/api/telemetry`,
`/api/service-archives/*`, terminal attach, and Secrets Broker proxy routes. Each needs a route
contract and an owning Core dependency before it is placed in an MVP phase.

**Rule:** direct `POST /api/services/:id/:action` is not used by this TUI. All lifecycle mutations
must use the operator/v2 preview/confirmation/readback flow. Admin-only convenience routes are not
substituted with client-side state.

## 6. Keybindings

**Global**
| Key | Action |
|---|---|
| `Ctrl+K` or `:` | Command palette (services, actions, screens, theme) |
| `g` then `d s p o l u m i v n y` | Go to Dashboard, Services, dePendencies, rOutes, Logs, rUntime, Mcp, Installed, Variables, Network, securitY |
| `g` then `b t a` | inBox, Telemetry, Audit |
| `Tab` / `Shift+Tab` | Cycle focus: sidebar → main → log dock |
| `/` | Filter current list |
| `` ` `` | Toggle log dock |
| `Ctrl+R` | Refresh now |
| `?` | Help overlay for the current screen |
| `Esc` / `q` | Back; `q` on root quits; `Ctrl+C` always quits |

**Lists and service detail**
| Key | Action |
|---|---|
| `↑↓` / `j k`, `PgUp PgDn`, `g g` / `G` | Move |
| `Enter` | Open |
| `s` / `S` | Start / Stop (stop confirms) |
| `r` / `L` | Restart / Reload (confirm) |
| `i` / `c` | Install / Config action |
| `f` | Toggle favourite |
| `o` / `y` | Open URL in browser / copy URL |
| `1`–`0` or `Alt+1`–`Alt+0` | Detail tabs (Admin uses Ctrl+1..0, which most terminals don't deliver) |
| `A` / `Z` / `Shift+R` / `Shift+L` | Start all / Stop all / Restart all / Reload runtime |

**Logs**: `f` follow, `t` type, `/` search, `n N` next/prev, `PgUp` older, `w` wrap, `>` send stdin line, `e` export to file.
**Terminal**: `Enter` attach, `Ctrl+]` detach.

---

## 7. Live updates
Core exposes **no SSE or WebSocket**; everything is polling, like Admin.
| Data | Interval | Notes |
|---|---|---|
| `/api/dashboard`, `/api/dashboard/services` | 3 s focused, 15 s idle | Admin uses 30 s; a TUI benefits from faster |
| Selected service detail | 2 s | plus immediate refetch after an action |
| `/api/metrics`, `/api/runtime/instance`, `/api/network`, inbox counts | 30 s | same as Admin |
| Log tail | 1 s while following | `GET /api/logs/read?limit=200`; append lines beyond last seen `totalLines`; no `after` param exists |
| Terminal output | 200 ms | same as Admin, `after=<sequence>` |
| Connection heartbeat | 5 s | `GET /api/health`; drives status bar and offline screen |

Rules: pause polling for hidden screens; exponential backoff (1→2→4→8 s, cap 15 s) on errors; never overlap requests; keep last good data visible and mark it stale.

## 8. Connection and auth
**Base URL discovery, first hit wins:**
1. `--api <url>` flag
2. `SERVICE_LASSO_API_BASE_URL` env (same name Admin's service.json uses)
3. `<workspace>/.service-lasso/runtime-instance.json` → `apiUrl` (`--workspace`, default cwd), only if `heartbeatAt` is fresh (Core lease TTL is 45 s)
4. `~/.service-lasso/instances.json` → live instances; picker if more than one
5. Probe `http://127.0.0.1:17883` then `http://127.0.0.1:18080` with `GET /api/health`, accept only `service: "service-lasso"`

**Auth:**
- Same machine on loopback: Core grants implicit `local-root`. Show the actor from `/api/runtime/security` in the status bar.
- Token: `--token`, `SERVICE_LASSO_LOCAL_ADMIN_TOKEN`, or a login screen that calls `POST /api/runtime/auth/local`; send `x-service-lasso-admin-token`. Keep the session in memory only.
- Never send Admin's proxy or trusted-ingress headers.
- Redact log lines the same way Admin's `redactLogLine` does; never print secret values.

## 9. Error and offline states
| Situation | Behaviour |
|---|---|
| Runtime unreachable at start | Offline screen (4.7) with the discovery trail, auto-retry, `[s]` runs `service-lasso start` (opt-in) |
| Connection lost mid-session | Status bar turns red "reconnecting"; screens keep last data dimmed with "stale 12s" |
| 401 / 403 | Login screen / "Forbidden: actor lacks permission X" toast; actions greyed |
| 404 on optional APIs (audit, inbox) | Screen shows "not available on this runtime" (Admin does the same) |
| 409 / 5xx on action | Toast with Core's `detail`/`message`; action row shows the failure |
| `isolation.startBlocked` | Start disabled with the `startBlockedReason` |
| Terminal unavailable or read-only | Show Core's `reason`; read-only hides input |
| Terminal too small (< 80×24) | "Enlarge terminal" notice |

## 10. Theming
- Tokens mirror Admin's `theme.css`: background, panel, border, text, muted, accent, success, warning, danger.
- Themes: Dark (default), Light, System (detect via `COLORFGBG` or OSC 11), High contrast. `NO_COLOR` honoured.
- Truecolor with automatic fallback to 256 and 16 colours (chalk level detection); Windows conhost falls back cleanly.
- Status: running green `●`, available cyan `○`, degraded amber `▲`, stopped grey `■`, critical red.
- Borders: rounded Unicode by default, `--ascii` for legacy consoles; optional Nerd Font icons.
- Settings saved to `%APPDATA%\service-lasso\tui.json` / `~/.config/service-lasso/tui.json`.

## 11. Project structure

Extend the existing TUI repository:
```
cmd/service-lasso-tui/        # binary entry, flags, terminal lifecycle
cmd/service-lasso-operator-probe/
internal/api/                 # operator/v2 client, types, validation
internal/app/                 # Bubble Tea model, views, key handling
internal/coreentry/           # retained native entry boundary
internal/lifetime/            # process/terminal ownership
scripts/local-e2e-pty.py      # Linux/macOS terminal proof
scripts/local-e2e-conpty.py   # Windows ConPTY proof
scripts/render-terminal-video.py
```
No `packages/tui` Node package and no duplicated TypeScript API client.

## 12. Phased build plan

**Phase 0 — shell and verified reads (already partly present):** connect via discovery, render the
sidebar/status bar, load services through operator/v2, and prove Linux PTY plus Windows ConPTY
readiness. Exit: no 404/500 from advertised reads.

**Phase 1 — MVP operator loop:** dashboard/service table/detail from operator/v2; lifecycle
availability → preview → confirmation → execute → readback; logs; offline/error states; help and
command palette. Exit: route-level E2E covers every Phase 1 screen and every lifecycle state against
a real Core.

**Phase 2 — read-only parity:** variables, endpoints, dependencies, network, routes, installed,
inbox, audit, history, and public graph. Only ship screens whose Core dependency is verified.

**Phase 3 — guarded writes and advanced views:** config, setup, declared actions, updates, removal,
terminal attach, MCP and security read-only views. Each mutation requires its own preview/confirm
contract and failure/readback test.

**Phase 4 — broker and parity polish:** Secrets Broker read-only screens, catalog, multi-instance
picker, themes, high-contrast, packaging.

Every phase needs a screen/state acceptance matrix: loading, empty, stale, unauthorized, offline,
error, and success. Unit tests alone do not close a phase.

## 13. Open questions and gaps

Status after reconciliation: the design is directionally valid, but it is not an implementation
contract until Section 5.1's unverified Admin routes have contracts and the Phase 1 matrix is green.
1. No push channel in Core (no SSE/WebSocket); all live data is polling. A `GET /api/events` SSE stream would make the TUI and Admin cheaper and snappier.
2. `/api/logs/read` pages backwards only (`before`/`cursor`); a forward `after` cursor would make tailing exact instead of diffing `totalLines`.
3. Loopback clients get implicit `local-root`; Admin gates this behind an explicit break-glass step. Should the TUI require `--token` or a confirm, or accept local-root silently?
4. "Restart all" isn't a Core action; Admin calls stopAll then startAll. Keep that or add `restartAll` in Core?
5. Admin calls `/api/service-archives/upload|import`, `/api/logs/content`, `/api/inbox/messages/:id` and `/api/services/@secretsbroker/{lifecycle,operations,providers,secrets,sources}/*`; I could not find the first two in Core's `src` on origin/develop (Core has `/api/v1/service-transfers`). Some are stub-only paths; confirm which are live before mapping them.
6. The TUI lives in this repository. A future Core `service-lasso tui` launcher would be an integration decision, not a new `packages/tui` implementation.
7. Which Secrets Broker actions are acceptable in a terminal (scrollback and recording risk)? The proposal is read-only with no reveal.
8. Should the offline screen be allowed to start the runtime itself (`service-lasso start`)?
9. Local checkout of Core is 67 commits behind origin/develop; endpoints here are from origin/develop.
