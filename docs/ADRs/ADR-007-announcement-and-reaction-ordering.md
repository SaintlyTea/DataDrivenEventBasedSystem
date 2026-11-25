# ADR-007: Announcement and Reaction Ordering

**Status:** Accepted

---

## Context

Some events represent actions that the rest of the game should be able to react to *before* they resolve — a spell being cast, an attack being declared. When such an event is about to execute, other events may want to respond to the mere announcement of it (buffs that trigger on any spell cast, counters that fire when an attack is declared), and those responses must resolve before the announced event itself does.

This creates two problems:

1. **Ordering** — the reactions to an announcement must run before the announcing event, but the announcing event must still run afterward, in the same burst, before unrelated pending work.
2. **Condition timing** — a reaction's condition may only become true *because an earlier reaction in the same announcement already ran*. For example, one reaction sets a target alight, and a second reaction deals bonus damage only if the target is burning. If reactions were filtered by their conditions at the moment the announcement is made, the second reaction would be wrongly excluded, because at announcement time the target is not yet burning. The combo the player reasonably expects would silently fail.

A naive approach of checking each reaction's conditions when the announcement fires breaks these combos. But simply never filtering risks announcements cascading — a reaction that is itself an announcing event would announce in turn, and if that is not handled carefully it can loop.

---

## Decision

### Announcement declaration

Each event has an announcement field, set when the event is defined. It holds the announcement trigger the event fires before executing, or is left empty if the event makes no announcement. When an event with a set announcement field is about to execute, that trigger is fired. An empty field skips the announcement step entirely.

### Reactions resolve before the announcer, via the LIFO lane

When an event about to execute is found to have an announcement it has not yet made:

1. The event is placed back into the LIFO lane.
2. The events listening to its announcement trigger are resolved and ordered (via the standard resolve-and-sort pipeline), then placed into the LIFO lane after it, arranged so the first reaction to process sits on top.

Because the lane is last-in-first-out, the reactions pop and execute first, in their sorted order, and the announcing event pops last — after all its reactions have resolved, but before any unrelated pending work. No separate announcement queue is needed; the existing LIFO lane provides exactly the required ordering.

### `has_announced` prevents re-announcing

The announcing event, once it returns to the top of the LIFO after its reactions have drained, must execute rather than announce again. This is tracked by a `has_announced` flag on the `QueuedEvent` wrapper, because the flag is a property of this specific queued occurrence of the event. When the event is revisited and `has_announced` is set, it skips the announcement step and executes normally.

### Announcement reactions are not condition-checked when the announcement is made

Reactions to an announcement are placed onto the queue without filtering them by their conditions at that moment. Their conditions (and cancellation) are checked at execution time, when they are popped — the same authoritative execute-time gate that governs all events. This is what makes mid-announcement combos work: a reaction whose condition only becomes true because an earlier reaction ran is still in the queue, and passes its check when its turn comes.

### Cascading announcements

A reaction may itself be an announcing event, producing its own announcement burst nested within the first; the same mechanism handles it recursively. Announcing executes nothing and stays at the same level, so neither width nor depth sees it; each handler therefore also counts announcements per event definition id against the width tolerance, and an event that exceeds it is banned as for width.

---

## Consequences

- Reactions to an announcement always resolve before the announced event, in deterministic sorted order
- Any event can allow others to react to its intent to execute, simply by naming a trigger others listen for
- Multi-step combos work as a player expects, because a reaction's eligibility is judged when it is popped — after earlier reactions have already changed the world — not when the announcement is made
- Some queued reactions never fire, since their conditions are judged at execution and may by then be false. Because width counts only events that actually execute, these never-fired reactions do not consume the width budget, though the width tolerance should still be generous enough not to cut short combos where one event legitimately executes many times at a level

---

## Alternatives Considered

- **Checking reaction conditions when the announcement is made, filtering out those that currently fail** — the intuitive approach, but it breaks any combo where one reaction only becomes valid because an earlier reaction ran first. The order-sensitive combos this excludes are exactly the kind of emergent interaction the system exists to support, so filtering at announcement time is rejected in favor of the authoritative execute-time check.
- **A dedicated announcement queue separate from the main lanes** — would work, but the existing LIFO lane already produces the exact ordering required (reactions first in sorted order, announcer last, all before unrelated work). A separate queue would add a structure and a set of rules for no behavioral gain.
- **Tracking "already announced" on the context rather than the queued event** — incorrect, because a context can be shared or referenced across events, so the flag would not reliably describe a single event's occurrence. The queued-event wrapper is the natural per-occurrence home for it.
- **A dedicated loop guard for announcement cascades** — unnecessary, since an announcement cascade is just nested queued work and the existing depth, visit-tolerance, and width limits already bound it. Adding a separate guard would duplicate protection that already exists.