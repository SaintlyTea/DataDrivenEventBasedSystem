# ADR-005: Type-Correct Context Construction and Chain Derivation

**Status:** Accepted

---

## Context

When an event is queued as a reaction to another event, it needs a context to execute against. That context must match the *reacting event's own type*, not be inherited wholesale from the event that spawned it. A combat event spawning a non-combat reaction cannot hand it a combat context (the reaction would carry fields that do not belong to it), and a non-combat event spawning a combat reaction cannot provide the combat-specific fields the reaction needs (it would be missing targets and similar data).

At the same time, some state genuinely should carry across contexts — the shared depth tracker that guards the whole run, propagating payloads such as cancellations, and, in some cases, fields shared by a family of related context types (for example a damage type shared across several damage-related contexts).

The problem is doing this without either (a) building a matrix of "how to derive type X from type Y" for every pair of context types, which does not scale, or (b) losing shared fields that legitimately should propagate between related types.

---

## Decision

### Type-correct construction via a dispatch helper

Context construction for a reaction is routed through a single helper that dispatches on the reacting event's type and builds the correct context type for it. The core calls this one helper. The mapping from event type to the concrete context builder lives in the shell, so adding a new event type with its own context means adding one case and one builder — the core is never touched. This mirrors the announcement-mapper pattern: one entry point in the core, the type-specific mapping in the shell.

### Chain derivation

Deriving a new context from an existing one walks the inheritance chain from the base down to the concrete type. Each level in the hierarchy is responsible only for its own layer of state:

- The root (`EventContext`) always handles the universal state — sharing the run's depth tracker and copying propagating payloads.
- Each intermediate level copies the fields it introduces (not all fields need to be copied), unconditionally for any context within its family (e.g. a shared damage type carried by all damage-related contexts).
- The concrete leaf level copies its own specific fields.

Because each level only knows about the state it introduces and delegates everything else upward, no context type ever references a sibling type. Shared fields propagate correctly between two different leaf types precisely because their *common ancestor* owns that field and handles it.

### Cross-type derivation is base-only, by design

When a context is derived from a parent of a *different* type, only the state owned by their common ancestors carries over — in the general case, just the universal base state. 

---

## Consequences

- A reaction always executes against a context of its own correct type, with no missing or foreign fields
- Shared state that should propagate (depth tracker, cancellations, family-shared fields) does so automatically through the ancestor that owns it, without any type checking against other types
- Adding a new context type means implementing only its own layer's derivation and registering its builder in the shell helper; existing types are untouched and the core is untouched
- No pairwise "derive X from Y" matrix exists or is needed, because cross-type derivation is always base-only and same-type/family derivation is handled by the owning ancestor
- Related context types share invariant identity fields (such as damage type) through a common ancestor. This is inheritance used for data-identity declaration. Mutable working data (modifiers and other values adjusted as a calculation proceeds) lives in payloads, which are composed and carried. This keeps the chain a statement of identity rather than a source of behavioral coupling. The hierarchy should stay shallow regardless, and levels should only ever declare their own fields and derive their own layer

---

## Alternatives Considered

- **A single derive method with an internal "if the types match" branch** — works, but the presence of the type-match check implies there is a meaningful "different type" path that could or should be filled in, which misleads future readers into thinking cross-type derivation is unfinished. Chain derivation removes the implication entirely: each level simply handles its own layer.
- **A pairwise derivation matrix (how to derive each type from each other type)** — maximally flexible but does not scale; every new context type would require considering its relationship to every existing one. Rejected because related types can share through a common ancestor instead, and unrelated types have nothing to share.
- **Inheriting the spawning event's context type directly** — simplest, but produces contexts of the wrong type for reactions (foreign fields present, or required fields missing), which is the core problem this ADR exists to solve.