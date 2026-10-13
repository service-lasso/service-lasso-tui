# Operator/v2 log search source

The current issue1/PR33 source remains incomplete and UNRUN. The selected v2
contract defines a log `search` query of at most 256 UTF8 bytes. Its actual Core
v2 log producer, source-handle catalog, tail ownership and installed/native
acceptance remain missing or unproved. This source does not use the historical
`/api/logs/search` route or its `q`/`query` fields.

In the Logs view with row focus, `/` opens the read-search editor. Enter applies
the literal query and starts a fresh first page; Escape retains the previous
query. Ctrl+u clears the editor; Enter then applies an empty query. Arrow keys,
Home, End, Backspace and Delete edit at grapheme boundaries. The query and
continuation stay bound to the current connection, service, stream and trusted
context. Responses and cursors from a previous query cannot become current rows.

Search has its own editor and preserves retained mutation inputs, operation IDs
and storage warnings. Controls are escaped for display without changing the
query bytes. `r` refreshes the current query and `n` continues only its admitted
snapshot. Search neither discovers an operation ID nor replays a mutation. A
missing log route or denied source remains an error; it is never empty success.

The new HTTP and reducer source cases are UNRUN pending a different fresh entire
source review and a new complete actual ROOT with exact-command admission.