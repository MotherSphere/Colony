# Archive Vault Synchronization Strategy

Archive Vault aims to support a range of synchronization backends without compromising the zero
knowledge security model.

## Provider abstraction

Implement `archive::sync::Provider` to unify lifecycle hooks (`Connect`, `Pull`, `Push`, `Resolve`)
so that file-based, WebDAV, or future cloud connectors share the same surface.

## Conflict resolution

- Track per-entry revision IDs and timestamps.
- Preserve server/client history to produce user-facing merge dialogs inside the dashboard.
- Defer secret material merging to the local device to keep master keys private.

## Roadmap

1. Ship a file-based provider (local folder or git repository).
2. Add WebDAV with optional mutual TLS for self-hosted setups.
3. Define REST/GraphQL API contracts for managed Colony backends.
4. Harden with integration tests and fuzzing for importer/exporter pairs.
