# Archive Vault Integration API

Archive Vault exposes a controlled interface so that other Colony programs can request secrets
without breaking isolation.

## Guiding rules

- Secrets never leave the vault unencrypted unless the caller holds a capability token.
- Access is auditable: every request, approval, and denial is logged to a local encrypted journal.
- Permissions are scoped per entry or tag.

## Proposed surface

```cpp
struct RequestSecret {
    std::string entry_id;
    std::string field_key; // e.g. "password", "totp"
};

class VaultBridge {
public:
    std::optional<std::string> FetchSecret(const RequestSecret& request);
    void PresentAuditLog();
};
```

`VaultBridge` will be backed by an IPC or in-process message bus to avoid tight coupling with the
UI implementation. Consumers must present capability tokens before a secret is released.
