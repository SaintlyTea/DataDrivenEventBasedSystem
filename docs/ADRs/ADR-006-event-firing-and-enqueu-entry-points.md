# ADR-006: Event Firing and Enqueue Entry Points

**Status:** Accepted

---

## Context

Events enter the system and get placed onto queues through several distinct paths: an external caller starting an action (a player casting a spell), an event firing a trigger mid-execution to gather reactions (a complex event consulting other events), and an event declaring a follow-up trigger after it runs. These paths share the same underlying need — take a trigger, find the events listening to it, order them correctly — but differ in what they do with the result: some start a fresh sub-run, others inject into the run already in progress.

Without a clear structure, this logic tends to get duplicated or tangled, and the ordering rules (which are correctness-critical, see ADR-004) risk being applied inconsistently across paths.

---

## Decision

Responsibilities are split into three layers: sorting, trigger resolution, and firing.

### Sorting — `EventSorter`

`EventSorter` owns a single pure operation, `sort(events, context)`, which applies the deterministic priority chain (ADR-004) to an already-gathered list of events. It takes a list and a context and returns the list ordered. This keeps sorting trivially testable and reusable for any list of events, regardless of where they came from.

### Trigger resolution — `get_sorted_events_for_trigger`

A shared step, `get_sorted_events_for_trigger(trigger, context)`, lives in the firing layer. It fetches the events listening to a trigger from `EventStore`, then delegates to `EventSorter::sort`. This is the common foundation both firing variants build on.

### Firing — two variants over a shared base

Both variants call `get_sorted_events_for_trigger` and then differ only in placement:

- **`fire_trigger(context, trigger)`** — creates a fresh `EventQueueHandler`, seeds the sorted events into its FIFO lane, and runs it to completion as a self-contained sub-run. Used when an event needs reactions gathered and fully resolved mid-execution (e.g. a complex event consulting modifiers or dependant on another events execution) — or to simply kick off a trigger.

- **`queue_trigger(context, trigger)`** — resolves the same way but places the sorted events into the *current* run's LIFO lane, so they run before the next pending event without starting a new handler. Used for follow-up triggers and any case where reactions belong to the run already in progress.

- **`gather_trigger(context, trigger)`** — as `fire_trigger`, but every event answering the trigger is a modifier: its context shares the caller's gather payloads (ADR-008). The caller reads the gathered result from those payloads once the sub-run completes.

### Event ignition — `fire_event` and domain wrappers

`fire_event` is the external entry point for starting an action from outside the system. It prepares the initial event's context, seeds it into a fresh handler, and runs. Because the fired event is always the first to execute, its conditions are checked before anything else can enqueue.

Domain-specific wrappers exist for cases where data has already been selected outside the system and must be carried into the event's context before firing — most commonly player-chosen targets, which cannot be inferred and must be passed in as a parameter. Such a wrapper takes that pre-selected data, places it into the context correctly for that domain, and then delegates to `fire_event`.

### Context construction and targeting

The context handed to an event must be suitable to the event's own type, not be inherited wholesale from whatever spawned it (see the context derivation ADR). Context construction is therefore routed through a type-dispatch helper that builds the correct context type for the event being enqueued. Each queued event carries its own context, built when it is queued from the context that caused it; an event placed on a queue without one receives it from the handler's context when popped.

Targeting follows two rules:

- **Explicit targets provided** (e.g. the player chose them): they are written into the event's context and locked — never re-resolved or overwritten, since a provided target represents a deliberate choice.
- **No explicit targets**: targets are resolved just before execution using the event's targeting strategy, so they reflect the freshest state and can fizzle cleanly if the intended target is gone by execution time.

---

## Consequences

- The correctness-critical ordering (ADR-004) is applied through one shared path (`get_sorted_events_for_trigger` → `EventSorter::sort`), so every firing path orders events identically
- Domains needing externally-selected data (such as player-chosen targets) get a thin wrapper that routes it into the context before firing. Domains needing no such data use `fire_event` directly, and the core firing logic is never modified in either case
- Explicit player-chosen targets are guaranteed stable through execution, while system-resolved targets stay fresh and fail gracefully — at the cost of two distinct targeting timings that authors must be aware of

---

## Alternatives Considered

- **One combined method that fetches, sorts, and places in every case** — simpler to call, but it would either duplicate the fetch-and-sort logic across the sub-run and current-run cases or bury a placement branch inside one method, making the two very different behaviors (start a new run vs inject into the current one) harder to see. Splitting the shared resolve step from the placement variants keeps each intent explicit.
- **Letting `EventSorter` fetch its own events from the store** — more convenient, but it would couple the sorter to `EventStore`, making it harder to test in isolation and impossible to reuse for sorting a list that did not come from a trigger lookup.
- **Resolving all targets at enqueue time** — would make targets available earlier, but system-chosen targets could then be resolved against a board state that changes before the event executes. Resolving at execution keeps them fresh; only deliberately provided (player-chosen) targets are locked early, because those represent a choice that should not silently change.