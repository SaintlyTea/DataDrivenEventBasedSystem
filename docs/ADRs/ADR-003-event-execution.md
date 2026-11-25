# ADR-003: Event Execution Model for Composite and Multi-Step Events

**Status:** Accepted

---

## Context

Simple events (e.g. `ApplyEffect`) need minimal orchestration — read their own data and act. Complex events (e.g. `DealDamage`) require multiple sequential steps: gathering damage modifiers from other events, calculating a result, gathering defense modifiers, calculating again, then applying the outcome.

An earlier design made complex events fully responsible for their own orchestration — manually building contexts, emitting triggers, and pushing follow-up operations onto the queue with the correct priority. This required any developer writing a complex event to understand the entire processing pipeline in depth, which contradicts the project's goal of allowing new events to be authored without deep knowledge of the core.

Two distinct cases were identified that the previous design conflated:

1. **Composite events** (e.g. `Sequence`), whose entire purpose is to split into multiple child events that must be queued in a specific order
2. **Single conceptual events with internal multi-step logic** (e.g. `DealDamage`), which are not a sequence of independent events, but one operation that needs to consult other events partway through its own execution

---

## Decision

Both cases reuse the same core primitives — `EventQueue` and `EventQueueHandler` — rather than introducing new mechanisms.

The queue does not hold bare events. Each entry is a `QueuedEvent`, pairing the event with the context it will execute against (its own context), the context it is reacting to (its triggering context, if any), and bookkeeping flags used by the handler. This lets each event carry the state relevant to it through the queue without polluting a shared context.

**Composite events** use a core-provided utility to queue their child events with the correct ordering, so the event author only declares the children and their relationship, not the queue mechanics.

**Single conceptual events with internal steps** call the trigger-firing entry point (see ADR-006 — Event Firing & Enqueue Entry Points) when they need reactions from other events mid-execution. That entry point resolves the eligible, ordered events for the trigger and runs them to completion in a fresh sub-run built for that purpose. The calling event places a gather payload (ADR-008) in the context it passes in, fires the trigger through the gathering entry point, and reads the result back from that payload.

This sub-run is always freshly created and discarded after use — never pooled — so a failure or early exit in one step cannot leak state into a later one.

To the outer queue, an event using this pattern appears as a single atomic unit of work, regardless of how much internal processing it performs.

### Cancellation and Condition Checking

`EventBase::execute()` always validates both cancellation and conditions before running. This execute-time check is authoritative and is the sole gate on whether an event runs — an event only executes if it is not cancelled and its conditions hold at the moment it is popped for execution.

Conditions are intentionally *not* checked when an event is placed on a queue. Checking at enqueue time would risk excluding an event whose condition only becomes true because an earlier queued event runs first — for example, a reaction that should fire once a target is set alight by an earlier reaction in the same batch. Deferring the check entirely to execute time guarantees no event is wrongly excluded by ordering.

A future option to also check at enqueue time — for the rare event that must qualify immediately or not run at all — may be added later. 

### Follow-Up Triggers

Events may define a `follow_up_trigger_` attribute, set at construction. After an event executes, if it declares a follow-up trigger, the handler resolves the events listening to that trigger (fetching and sorting them the same way any trigger firing does) and places the resulting ordered events into the current run's LIFO lane via `queue_trigger`, so they run before the next pending event. The executing event itself does not enqueue these reactions — it only names the trigger and writes any data later events need into the context before returning.

The distinction between firing a trigger into a fresh sub-run and queuing a trigger's events into the current run is specified in the firing entry-points ADR. Follow-ups use the current-run form; mid-execution sub-runs (such as a complex event gathering modifiers) use the fresh sub-run form.

### Safety Limits

Sub-runs are bounded by a depth tracker shared across the entire run (via the context), guarded with RAII(Resource Acquisition Is Initialization) so the depth is correctly decremented even if a step exits early or throws. The tracker enforces:

- **Maximum depth** — a hard ceiling on how deeply sub-runs may nest, catching chains that nest arbitrarily deep across different events (A triggers B triggers C triggers A...).
- **Per-depth visit tolerance** — each depth level tracks how many times it has been entered before the run fully unwinds. The tolerance for a level decreases as depth increases (slowly at first, accelerating, with a floor), catching loops that oscillate between levels without ever hitting the maximum depth. Visit counts reset only when the run fully unwinds — that is, only when depth returns to the top-most level (-1) and the handler is ready to pop its next event — so genuinely long but finite chains are usually not penalized while true loops are caught.
- **Width** — guards against an infinite loop occurring *at a single level*, where one event's execution re-queues work that leads back to that same event executing again on the same level without ever descending or resolving (A queues B, B queues C, C queues A, all at the same level). Rather than a flat impatient cap, width mirrors the depth tolerance approach: it counts how many times an event with a specific ID has executed at the same level and allows that up to a tolerance, so long legitimate chains are not cut off prematurely while genuine same-level loops are still caught.

When a limit is hit, the run does not crash. It logs the condition and unwinds gracefully — the offending handler stops and control returns to the level above without executing the offending step.

#### Known limitation: blunt unwinding on limit hit

When a depth or visit-tolerance limit is hit, the current behavior stops the offending handler and unwinds. This punishes innocent work: legitimate events waiting at a level above a loop lose their chance to run because the whole subtree is torn down, even if they only needed one clean trip through the affected level.

Current mitigation: for width specifically, the first event (by definition id) to hit the limit at a level is banned from further execution at that level via a ban payload, rather than tearing everything down. This lets the rest of that level's legitimate work still resolve. The ban payload is shared across the whole run. The handler that issued the ban runs that definition no further; every other handler allows a banned definition a configurable number of executions before skipping it, with that count reset when the handler finishes.

Deferred, harder problem: in a genuine multi-event loop (A -> B -> C -> A) there is no single clear offender, so deciding which event to ban is not obvious — banning the wrong participant can kill a legitimate combo while the runaway reforms through another path. A clean, general solution to identifying what to stop is wanted; the approach is open, as long as it stops the loop without needlessly killing legitimate surrounding work.

Related future concern: an infinite or near-infinite combo that creates persistent objects each iteration could exhaust memory before a limit stops it. Guarding object creation likely belongs at the object-creation method, outside event execution.

---

## Consequences

- Event authors do not need to understand queue ordering or trigger propagation to write either composite or complex single-purpose events
- The same processing primitives are reused everywhere; there is no second mechanism for "synchronous" triggers
- Carrying context per `QueuedEvent` rather than through one shared context keeps each event's execution state isolated, at the cost of a small wrapper object per queued event
- `check_condition_on_enqueue` gives fine control over whether an event that currently cannot fire should still occupy a queue slot, without ever risking correctness, since execute-time checks remain authoritative
- The boundary between a composite event and a single event with internal steps is a judgment call left to the event author. As a guideline: use a **composite** when two or more otherwise-unrelated events simply need to run together as a chain (or another arrangement its composite type defines); use a **single event with internal steps** when the event needs information from other events that it cannot otherwise easily obtain, or otherwise depends on another event's outcome to complete its own work.
- The depth, visit-tolerance, and width limits could, in rare legitimate cases, reject deeply nested or long-running but non-buggy behavior; limits and the tolerance curve are configurable rather than hard-coded, and a future per-event state-progress hook is noted as a possible refinement for distinguishing legitimate long chains from true loops

---

## Alternatives Considered

- **Making every event orchestrate its own multi-step logic by hand** (the original design) — the most flexible, but it forced anyone writing even a moderately complex event to understand the entire queue and trigger pipeline. This defeated the goal of letting people author events without knowing the core internals.
- **Passing computed results forward by attaching them to lightweight operation objects between steps** — this works when a complex event is a simple straight line of steps handing data to the next, but it falls apart for events that need to consult *other* reacting events partway through, and it still forces the author to manage queue placement by hand.
- **Adding a second, separate "run this trigger immediately" mechanism** alongside the normal queue — this would mean triggers behave in two different ways depending on how they were fired, which is more for a newcomer to learn and understand, with no benefit over simply reusing the existing queue and handler for sub-runs.
- **Keeping an always-on enqueue-time condition check** — an enqueue-time check is not fundamentally blocked by missing context — events queued as reactions carry the triggering event's context, and the event fired directly into a run is always the first to execute (its conditions are checked before anything else can enqueue). The real obstacle is consistency: a condition may query context-specific data such as a target, but not every context type has such data (a non-combat context has no targets). An enqueue-time check would therefore either need substantial plumbing to route the right information to the right place, or would have to skip context-dependent queries — which raises the question of how meaningful a partial check would even be. For that reason it is deliberately left out for now, with execute-time checking as the sole authoritative gate. It may be revisited if a clean way to handle the inconsistency emerges.
- **Catching infinite loops with a simple fixed count limit** — a plain "stop after N iterations" cannot tell a legitimate long combo apart from a true infinite loop: set it low and real play gets cut off, set it high and loops run too long before being caught. The depth limit, per-depth visit tolerance, and per-level width tolerance are used together instead, and a future per-event "is this actually making progress" (Do states change in a meaningful way) check is noted as a way to distinguish long-but-finite chains from real loops even more precisely.