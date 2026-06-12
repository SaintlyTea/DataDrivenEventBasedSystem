# DataDrivenEventBasedSystem

A work-in-progress **data-driven, event-based gameplay system**, primarily intended as a foundation for **turn-based games**.

This repository contains the system and a set of **small example datasets and demos** to show how it can be used (combat, UI, input, etc.).  
It is **not a complete game** and **not a full engine**.

---

## Work in Progress

This project is actively under development.

- The current codebase is **incomplete and unstable**.
- APIs, naming, and internal structures are subject to change.
- Some parts are experimental or partially implemented.
- A stable and fully documented release does **not** exist yet.

If you are looking for a production-ready library: this is not it (yet).

---

## Goal

The main goal is to provide a reusable base for games where gameplay logic is expressed through **data + events** instead of hard-coded, tightly coupled systems.

The system aims to make it easier to:
- Add or change abilities, events, characters, and interactions by editing data and adding event types
- Keep complex gameplay logic maintainable as it grows
- Build gameplay features in a way that remains testable and understandable

While the main focus is turn-based gameplay, the design may later be extended for other game types (e.g. tick-based) or even non-game systems.

---

## Non-Goals

This project does **not** aim to:
- Be a complete game or a playable product
- Replace existing game engines (Godot, Unity, Unreal, etc.)
- Provide a polished editor experience or production-level content workflows
- Guarantee performance optimizations at this stage (correctness and clarity come first)

---

## Example (High-Level): Firebolt

To make the intent concrete, here is a simplified example of how gameplay behavior is meant to be expressed.

**Firebolt** is an ability implemented as a **composite event** (specifically a *Sequence*):
- _DealDamage_
- _ApplyStatusEffect_ (Burning / damage-over-time)

**Flow (simplified):**
1. Player casts Firebolt (target chosen).
2. The system emits an announcement trigger:  
   _OnAbilityCastAnnounced_
3. All events that react to the announcement (and meet their requirements) are enqueued.
4. The Firebolt event itself is enqueued.
5. The queue is processed deterministically.
6. When Firebolt executes, it expands into its child events:
   - _DealDamage_
   - _ApplyStatusEffect_
   These child events are executed as an uninterrupted chain (no unrelated events between them).

This example is intentionally small. The repository will later include concrete datasets and tests demonstrating it.

---

## Examples and Demos (Why UI/Input/Assets exist here)

To validate that the system can actually drive gameplay, the repo will include minimal demo scaffolding such as:
- basic input handling
- minimal UI rendering for example screens
- lightweight asset/data loading required for demos

These exist to **run small tests and examples** and to observe the system in action.  
They are not intended to become a complete engine or a full production pipeline.

---

## Language and Future Direction

The system is currently being developed in **Godot (GDScript)** for fast iteration.

Once the design reaches a sufficiently stable and proven state (criteria not yet defined), it is being considered to either:
- rewrite the system in **C++**, or
- create a separate C++ repository implementing the same core ideas

No decision has been finalized yet.

---

## Documentation

Documentation will be written incrementally as the system stabilizes.

At this stage, documentation focuses on:
- goals and boundaries
- example scenarios
- design decisions (as they arise)

A more formal “Core Concepts” document will be added once the core system is stable enough that the explanations won’t become outdated every week.

---

## License

Licensed under the Apache License, Version 2.0
