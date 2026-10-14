# Typed error HTTP and retryability amendment — prospective, not adopted

Parent selection and coherent governance adoption precede new v2 error-router code. Original five and four prior amendments remain byte-exact. CODECS11 enumerates25 codes and HTTP categories but does not give a per-code HTTP/retryability/messageKey table. Actual server/index.ts2082–2094 durableLifecycleOperationError maps legacy typed owner codes, including all confirmation-prefixed errors to409 before its unavailable branch and fallback400; it must remain authentic v1 and cannot supply v2 semantics by string prefix/suffix. operator-v2-codec.ts9–18 currently has only the selected closed code enum/codec error, no installed typed v2 HTTP owner. This table is the additive explicit v2 selection, not a claim the router already exists.

Error envelope remains exactly {contractVersion,requestId,error:{code,messageKey,retryability,fieldErrors}}, within8192UTF8bytes. messageKey is EXACT `operator.error.<code>` with the listed code spelling; it is a closed display lookup key, not raw exception text, path, stdout, actor or diagnostic secret. English/client presentation can change without changing code/status/retry policy or admission. Unknown key/code/mismatched key/status/retryability/extra property is invalid. HTTP status is transport metadata and is not added to JSON. fieldErrors has at most32 exact {selector,code} objects; selector is an actual approved bounded logical field/operation selector, not arbitrary private path/value; code comes from the same25-code enum. Missing field-specific diagnostics uses [] rather than invented selector.

| code | HTTP | retryability |
| --- | --- | --- |
| invalid_request | 400 | never |
| invalid_utf8 | 400 | never |
| duplicate_key | 400 | never |
| limit_exceeded | 400 | never |
| unsupported_version | 400 | never |
| unsupported_capability | 422 | never |
| unsupported_selector | 422 | never |
| validation_failed | 422 | new_preview |
| authentication_required | 401 | never |
| permission_denied | 403 | never |
| confirmation_required | 409 | new_preview |
| confirmation_expired | 409 | new_preview |
| binding_stale | 409 | new_preview |
| revision_conflict | 409 | new_preview |
| operation_conflict | 409 | never |
| operation_unknown | 503 | never |
| source_unavailable | 503 | read_retry |
| source_corrupt | 503 | read_retry |
| snapshot_expired | 410 | read_retry |
| source_replaced | 409 | read_retry |
| storage_unavailable | 503 | read_retry |
| dependency_blocked | 422 | new_preview |
| ownership_unknown | 503 | never |
| not_found | 404 | never |
| internal_error | 500 | never |

The table refines the selected categories: malformed/unsupported wire-version400, authentication401, permission403, positively absent declared resource404, binding/claim/confirmation409, expired read snapshot410, supported request's domain validation/capability/selector/dependency422, source/storage/ownership uncertainty503, unclassified internal500. Unsupported capability is an otherwise well-typed requested action not implemented; an unrecognized malformed action ID remains invalid_request400. Missing source inspection is unavailable503, never not_found404/empty. Wire limits alone use limit_exceeded400; private live claim capacity must be classified by its actual typed unavailable owner, not used to evict original claims.

Retryability is an instruction about safe NEXT steps and NEVER permission to replay a mutation. never means no automatic retry; genuine manual fresh read/reconnect/repair can still be attempted under current entitlement, without forgetting unknown original operation authority. read_retry allows only a fresh currently authorized read/acquisition/reconciliation; it never resends POST/PATCH, reuses stale cursors or turns a read success into mutation success. Snapshot expiry/replacement obtains a new snapshot from its genuine owner. source_corrupt/storage_unavailable can be read again after actual repair but original evidence remains held. new_preview invalidates the old preview/confirmation and allows fresh same-context form/read/full validation/preview after the user corrects or the source owner changes; it does not automatically POST, acquire a new idempotency claim or abandon a possibly executed operation.

Mutation transport ambiguity or known post-claim uncertainty stays operation_unknown/ownership_unknown with existing retained exact operation/root/context authority and no replay. Do not relabel it revision_conflict/new_preview or source_unavailable/read_retry to restart the mutation. A typed definitive pre-effect validation denial can use the corresponding listed code only when the actual owner proves no claim/effect was admitted. If execution may have happened and no valid operation ID was delivered, the client enters unknown_no_id and retains read-only context as selected; no error table makes missing ID a retryable submission. With a valid ID, current exact-own GET can reconcile genuine status/result even when the prior error's mutation retryability is never.

Server producers classify their actual source/domain predicates, typed guarded owner results and durable claim phase through an exhaustive checked adapter. No prefix/suffix/regex/message inference, human skippedReason mapping, errno-only absence or arbitrary cast. Existing v1 error bodies/statuses remain unchanged. V2 unknown/unclassified causes map internal_error with no raw detail while private original diagnostics and uncertainty custody remain retained. Authentication/profile/scopes/source/manifest checks precede source details according to the whole selected contract; this table does not bypass that order.

Whole fresh independent review must cover actual router/error owner, closed Core/TUI status/key/retry/field parity, genuine pre-effect versus post-claim error classification and retention/reconciliation paths, all46/full compiler/K2/all9/native/provider obligations. Design only: no SOURCE_GO, target execution, native admission, repin, publication or GA.
