# ADR-002: Expression Atom Parsing

**Status:** Accepted

---

## Context

Expression atoms need to evaluate conditions against live game state — unit stats, statuses, board state, item properties, and similar data. A central state object with knowledge of everything was considered but rejected, since it couples every system together and grows without bound.

A first design encoded both the subject ("who") and the property ("what") directly into the atom type, producing one type per combination (e.g. `TARGET_HP_COMPARE`, `OWNER_HP_COMPARE`, `ANY_UNIT_HP_COMPARE`). This does not scale, since every new combination of subject and property requires a new atom type.

A second design kept a single atom type but encoded fields positionally in the atom value as a comma-separated string. This proved fragile: not all conditions need the same fields (some require a subject and operator, others only a status name), and positional parsing breaks silently when fields are reordered or omitted.

---

## Decision

Atom values are parsed as key-value pairs rather than positional fields:
{"STAT_COMPARE", "category:UNIT,subject:TARGET,stat:HP,op:<,value:50,unit:p"}

Resolution is split into independent, composable concerns:

1. **Field resolution** — atom values are parsed into named fields rather than positional ones. Each atom type defines its own set of expected field names, since different conditions need different information — `STAT_COMPARE` needs a category, a subject, an operator, and a value; `TIME_COMPARE` needs a time unit and a value to match; `FLAG` needs only a name and a boolean. There is no fixed schema shared across all atom types beyond "at least one field." Where a field refers to an event participant (e.g. a subject like `TARGET` or `OWNER`), it is resolved directly from the `EventContext` where possible, falling back to the relevant store for references outside the event's known participants.
2. **Data lookup** — a small interface (e.g. `IUnitDataLookup`) per data category, with one concrete implementation per category (`UnitStatLookup`, `ItemStatLookup`, etc.). Lookups are stateless and query the relevant store (`UnitStore`, `ItemStore`) for the actual data, identified by instance id so that multiple instances of the same data-defined entity (e.g. two bandits sharing one database entry) remain distinguishable.
3. **Comparison** — a small, fixed set of generic operators, reused across all stat types and data categories.

Lookup implementations are registered once at startup through category-specific registration functions (e.g. `register_unit_lookups()`), which are called from a single `register_all()` entry point. This keeps registration visible in one place without clustering every lookup into a single large file.

Registers and lookups hold no game state themselves — they are thin, stateless routers. Game state lifetime is owned entirely by the relevant stores, consistent with the lifetime model already used in `ExpressionStore`.

---

## Consequences

- Adding a new stat or data source requires only a new lookup implementation and one registration call, not a new atom type or comparison function
- Adding a new subject scope requires extending one resolver, not every existing atom type
- Required and optional fields can coexist in the same atom type without positional fragility
- Resolving known event participants (target, owner, source) avoids a store lookup entirely, keeping the common case fast
- Lookup helpers are effectively free to keep alive permanently, since they hold no per-instance state
- Atom values are no longer self-descriptive from the type name alone. Understanding a condition requires knowing the expected fields for that atom type, which should be documented per atom type as they are added

---

## Alternatives Considered

- **Central state object** — simplest to query but couples every system together and grows without bound
- **One atom type per subject/property combination** — readable per atom but combinatorially unsustainable
- **Positional comma-separated fields** — compact but fragile; breaks silently on reordering and cannot express optional fields cleanly