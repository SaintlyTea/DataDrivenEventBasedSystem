# ADR-004: Deterministic Ordering for Events Reacting to the Same Trigger

**Status:** Accepted

---

## Context

When multiple events react to the same trigger, the order in which they are queued and executed matters: one event's execution can change state (stats, statuses) that another event's conditions depend on. Without a defined ordering, events would queue in registration order, which is not guaranteed to be stable across sessions, save/load cycles, or when new instances are created at different times.

This creates a correctness problem, not just a cosmetic one: a combination of events that worked in one session could silently behave differently in another, purely because of incidental differences in when objects happened to be created or loaded — with no relationship to game logic the player can observe or reason about.

Owners and targets are not assumed to be units specifically. "Owner" refers to whatever object type owns events in a given domain, and "target" only applies to event/context types that have such a concept (e.g. combat events); other domains may have neither.

---

## Decision

Events optionally define an explicit **priority** value (higher value resolves first). Events with no explicitly assigned priority are treated as having a default mid-range priority, not zero — so an unset priority is neither the most nor least important, and explicit priorities can be set above or below the default as needed.

When multiple events are queued in response to the same trigger, they are ordered using the following chain, evaluated in order until a difference is found. Levels that depend on a concept not present for a given event or context type (e.g. a target, where the relevant context type has no notion of one) are skipped, and the chain proceeds to the next applicable level:

1. **Priority** — explicit, author-assigned value
2. **Owner of the trigger source matches the owner of the event** — an event owned by whoever caused the trigger resolves before one that is not
3. **Owner of the trigger target matches the owner of the event** — only applicable where the triggering context has a concept of a target; an event owned by the trigger's target resolves before one belonging to target
4. **The event's own definition id** — stable per database/data-file entry, regardless of how many instances exist
5. **A type-specific comparison, if defined** — event types may optionally override a `compare_to(other)` method to express a preference between two instances of themselves sharing the same definition id and owner. A composite event may delegate this to a child event that has a concretely comparable value (e.g. `DealDamage` comparing its damage amount). Default behavior is no preference, in which case the chain proceeds to the next level.
6. **The owning object's definition id** — stable per "kind of owner," regardless of instance creation order
7. **Registration order within the owning object's own list of events** — resolves ties when one owner has multiple events sharing the same definition id (e.g. two different `Sequence` composites, both typed `Sequence`, distinguished only by their position in that owner's own list)
8. **Instance id** — the final fallback, used only when two different owners are identical at every level above

This chain only affects ordering within the normal (FIFO) queue. Barrier (LIFO) operations are unaffected, since they are intended to execute immediately regardless of sibling ordering.

---

## Consequences

- Execution order for a given combination of events and owners is stable across sessions, save/load cycles, and instance creation order in the overwhelming majority of cases
- A new instance (e.g. a newly spawned enemy of an existing type) does not unpredictably out-prioritize or under-prioritize existing instances of the same kind in any case where some other distinguishing signal exists
- The chain resolves to something meaningful at every level except the last, giving content authors a way to reason about and predict ordering without needing to inspect instance-level details
- **The optional `compare_to` step (level 5) gives individual event types a way to express ordering preferences between their own instances without requiring every event type to define one.** It only resolves ties already narrowed down to the same definition id and owner, keeping responsibility scoped to "how do I compare to myself," not "how do I compare to anything else in the system." A single fixed `compare_to` per event type cannot express opposite preferences for two different abilities built from the same underlying event type (e.g. two different abilities both using `DealDamage`, where one wants lower damage first and the other wants higher damage first) — this is a known limitation, and a more expressive, data-authored comparison (e.g. specifying a field, a direction, and a weight per ability) is a possible future extension, not yet designed.
- **Instance id as a last resort is acceptable, unlike as a primary tiebreaker:** by the time level 8 is reached, every other signal — priority, relationship to the trigger, definition id, type-specific comparison, and position in the owning object's own list — has already failed to distinguish the two events. This only happens when two owners are genuinely identical in every way the system and the player can observe (e.g. two bandits with identical stat blocks and identical event lists). In this situation, no tiebreaker can produce an outcome the player could predict or learn, since there is nothing distinguishing the two from the player's perspective in the first place. Using instance id here is not worse than any alternative — it is simply the only remaining signal that is actually different between them.
- **Known limitation:** if two independently authored pieces of custom content (e.g. user-created abilities) are owned by different owners and tie through every level of the chain up to instance id, the final instance id assigned to each is still effectively incidental to when each was created, but by this point the ambiguity is unavoidable rather than a flaw in the ordering design.

---

## Alternatives Considered

- **Instance id as the primary tiebreaker** — rejected. Unlike its use as a final fallback, using instance id earlier in the chain would let *incidental* differences (such as spawn order between two otherwise-distinguishable instances) silently change outcomes the player could have learned and relied on, such as a known combo's behavior changing simply because a new enemy was created at a different point in a new session. This is a real, observable inconsistency, not merely an academic edge case, and is the reason instance id is placed last rather than first.
- **Board/slot position as tiebreaker** — would be stable and meaningful, but assumes a board-like structure that not every game built on this system will have
- **Deterministic hashing of event attributes** — does not actually resolve ties between events that are already identical in every attribute being hashed; only useful when combined with some already-distinguishing input, which does not exist in the hardest cases
- **A single global, generic comparison mechanism covering all event types** — not adopted as the sole mechanism, since different event types have fundamentally different comparable data (or none at all for some composites). The optional `compare_to` override (level 5) achieves the same goal without requiring every event type to participate.
- **Ability-specific, data-authored comparison syntax** — a fixed `compare_to` per event type cannot express opposite ordering preferences for two different abilities built from the same underlying event type (e.g. two abilities both using `DealDamage`, where one wants its lower-damage instance to resolve first and the other wants the opposite). A richer mechanism authored per-ability rather than per-event-type — for example, specifying a field to compare, a comparison direction, and a weight — could resolve this, but represents a small comparison DSL living in data rather than code. Not implemented now; left as a possible future extension once a concrete need for it arises.