# ADR-003: Event Execution Model for Composite and Multi-Step Events

**Status:** Accepted

---

## Context

Simple events (e.g. `ApplyEffect`) need minimal orchestration — read their own data and act. Complex events (e.g. `DealDamage`) require multiple sequential steps: gathering damage modifiers from other events, calculating a result, gathering defense modifiers, calculating again, then applying the outcome.

An earlier design made complex events fully responsible for their own orchestration — manually building contexts, emitting triggers, and pushing follow-up operations onto the queue with the correct barrier/normal priority. This required any developer writing a complex event to understand the entire processing pipeline in depth, which contradicts the project's goal of allowing new events to be authored without deep knowledge of the core.

Two distinct cases were identified that the previous design conflated:

1. **Composite events** (e.g. `Sequence`), whose entire purpose is to split into multiple child events that must be queued in a specific order
2. **Single conceptual events with internal multi-step logic** (e.g. `DealDamage`), which are not a sequence of independent events, but one operation that needs to consult other events partway through its own execution

---

## Decision

Both cases reuse the same core primitives — `EventOpQueue` and `EventQueueHandler` — rather than introducing new mechanisms.

**Composite events** use a core-provided utility to queue a list of child events onto the *outer* queue with the correct priority, so the event author only declares the children and their relationship, not the queue mechanics.

**Single conceptual events with internal steps** call a static core method, `EventTriggerHandler::run(context, trigger)`, when they need reactions from other events mid-execution. This method:

1. Looks up all events registered to the given trigger
2. Creates a fresh `EventOpQueue` and `EventQueueHandler`
3. Calls `enqueue()` on each matching event (which may or may not check its own conditions, depending on that event's own implementation — see Enqueue-Time Condition Checking below)
4. Runs the sub-handler to completion
5. Returns control to the caller, who reads results back from the context it passed in

To the outer queue, an event using this pattern appears as a single atomic unit of work, regardless of how much internal processing it performs.

### Enqueue-Time Condition Checking

`EventBase::execute()` always re-validates conditions before running, regardless of what happened at enqueue time. Whether `enqueue_()` also checks conditions before adding itself to a queue is left to each event type to decide:

- Checking conditions at enqueue time avoids queuing work that will just be rejected at execute time, but risks skipping events whose conditions would become true by the time their turn to execute arrives, if earlier queued events change relevant state first
- Skipping the check at enqueue time guarantees no event is incorrectly excluded due to ordering, at the cost of queuing some events that may ultimately do nothing

Event types that should always behave one way can override `enqueue_()` directly. For cases needing per-instance control, a constructor flag may be used instead of requiring a subclass.

### Follow-Up Triggers

Events may define a `follow_up_trigger_` attribute, set at construction like `trigger`. After `EventQueueHandler` executes an event, it checks this attribute; if non-empty, it looks up and enqueues matching events with the trigger onto itself automatically. The event itself does not enqueue these reactions — only sets the trigger name and writes any data later events might need into the context before returning.

### Safety Limits

Two independent counters guard against runaway recursive or looping behavior:

- **Depth** — a single counter shared across the entire run, incremented when any handler creates a sub-handler and decremented when it returns. Catches event chains that nest arbitrarily deep across different events (A triggers B triggers C triggers A...).
- **Width** — local to each handler instance, incremented each time that specific handler creates a sub-handler during the processing of its current event, and reset when that handler successfully pops its next event. Catches a single level looping on sub-handler creation without ever resolving.

Exceeding either limit aborts execution with an error rather than allowing the run to hang indefinitely.

---

## Consequences

- Event authors do not need to understand queue priority or trigger propagation to write either composite or complex single-purpose events
- The same processing primitives are reused everywhere; there is no second mechanism for "synchronous" triggers
- The boundary between "this should be a composite event" and "this should be a single event with internal steps" is a judgment call left to the event author, not something the core enforces. Though as a guideline, if two events need data from each other it's a single event with internal steps
- Depth and width limits introduce hard caps that could, in rare legitimate cases, reject deeply nested but non-buggy behavior; limits should be configurable rather than hard-coded constants

---

## Alternatives Considered

- **Fully manual orchestration per event** (original design) — maximum flexibility, but requires deep knowledge of the core to write any non-trivial event
- **Passing computed data forward via typed `EventOp` payloads between phases** — workable for strictly linear handoffs, but does not address composite or reactive modifier events, and still requires the event author to manage queue mechanics directly
- **A separate synchronous trigger-emission mechanism** distinct from the queue — would create two different behaviors for triggers depending on context, increasing conceptual surface area for no real benefit over reusing the existing queue and handler