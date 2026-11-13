# Operator metadata source behavior

The selected v2 metadata form reads canonical service metadata under the trusted
connection context. A ready form keeps its separate metadata source revision and
observed service generation. Editing a favorite preserves layout; editing layout
preserves the favorite. `automatic` supplies an explicit nullable layout reset.
Submission rereads metadata and observed generation and retains the draft on a
conflict. It does not replay an uncertain publication.

The overview shows a favorite as true, false or unknown. `F` changes a local
display filter: ready false preferences are excluded; unknown preferences remain
visible. Server service filters and paging queries remain canonical. Graph rows
show a layout only from their own current read acquisition; a new page request,
refresh or context change makes old acquisition labels unknown.

Source-only status: selected v2 ready metadata requires the authentic server
revision owner binding generation, held preimage/absence and persistent epoch,
with every writer participating. That producer and all-writer authority remain
unavailable/unproven. The current client composition neither issues metadata
revisions nor proves ABA, atomic publication, durability or current mutation
readiness. There is no legacy metadata fallback.

The private in-memory cache retains successful generation-key observations when
later reads fail or report corrupt state. A selected finite retention lifecycle,
capacity accounting and original diagnostic owner remain unsupplied; no cap,
TTL, safe eviction or durable capture is claimed. All new source cases are
UNRUN. Useful identical installed Core/CLI/TUI Windows/Linux acceptance remains
unfinished; macOS is Deferred/N/A.
