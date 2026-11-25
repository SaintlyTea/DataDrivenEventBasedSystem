# ADR-008: Cancellation and Propagating Payloads

**Status:** Accepted

---

## Context

An event may need to prevent another event from executing — a counter that stops a spell, an effect that negates an incoming action. The event being cancelled may sit anywhere in the queue, and may be queued through several different paths, so the cancellation must reach it wherever it is and whenever it is finally popped for execution.

Payloads carried by a context normally stay within that context. But some data must survive across the context boundary when a derived context is created for a reaction or sub-run — cancellation is one such case, and others may follow. A general mechanism is needed so that specific payloads can opt into propagating, rather than hardcoding each cross-context concern.

---

## Decision

### Propagating payloads

A payload type may declare that it propagates, and how: either as a shared reference, so all contexts see and mutate the same payload, or as an independent copy, so a derived context receives its own snapshot. When a context is derived from another, each propagating payload is carried into the new context according to its declared mode. Non-propagating payloads are left behind.

A third mode, gather, serves payloads that collect contributions from modifier events. A gather payload is shared by reference only with the events of a gathering sub-run fired from the context that holds it; every other derivation leaves it behind, so the events those modifiers trigger, and their own sub-runs, never receive it. A modifier that gathers for itself places a new payload in a context of its own, gathers into that, and writes its result into the payload it was given.

Propagation and its mode default to off — a payload that does not propagate declares nothing.

### Cancellation as a propagating payload

Cancellation is expressed as a propagating payload rather than a field on events, and propagates by shared reference so that a cancellation issued at any depth is visible to every context sharing it, including levels above the one that issued it.

It records cancellations in two forms: by instance id, cancelling one specific event occurrence, and by definition id, cancelling every occurrence of an event built from that definition. A cancellation may be permanent or one-time.

`EventBase::execute()` checks cancellation before running an event, blocking it if either its instance id or its definition id is cancelled. A one-time cancellation is consumed when it blocks an event; a permanent cancellation remains and blocks every matching occurrence.
---

## Consequences

- An event can be stopped from any point in the chain, and because cancellation propagates by shared reference, a cancellation issued deep in the chain reaches events queued at higher levels
- Either a single specific occurrence or every occurrence of an event definition can be cancelled, letting an effect block one cast or all casts of a given event
- Cross-context concerns beyond cancellation can be added by marking a payload type as propagating and choosing its mode, without changing the derivation logic or the context types
- Every root context starts with an empty cancellation payload that its derived contexts share, so a cancellation issued before or after a derive reaches all of them; for an event that is never cancelled the check finds nothing
- A payload type choosing between shared-reference and copy propagation gains flexibility at the cost of requiring the reader to know which mode a given payload uses. The mode is declared on the payload type itself to keep that choice visible