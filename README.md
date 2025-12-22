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
The system is currently developed in **Godot (GDScript)** as a reference implementation.

A decision has been made to **progressively rewrite the core system in C++**.  
This repository will gradually transition toward a C++ codebase, replacing GDScript components step by step as the architecture stabilizes.

Once the rewrite is complete, a **thin integration layer** will be introduced to connect the C++ backend to **Godot**, which will be used exclusively for:
- UI examples
- input handling
- visual and interaction-driven demonstrations

The long-term goal is to keep the **core system engine-agnostic**, with Godot acting only as a presentation and interaction layer.

### Why C++
The decision to move the core system to **C++** is driven by long-term goals rather than immediate necessity.

The system is intended to serve as a **general-purpose, engine-agnostic backend** for turn-based (and potentially other) systems. C++ provides:
- greater control over performance and memory,
- broader applicability outside of Godot,
- easier reuse in other engines or standalone simulations,
- and a more stable foundation for complex, long-lived systems.

Godot will remain part of the project, but only as a **presentation and integration layer** rather than the owner of the core logic.

### Transition Strategy
The repository is currently in an **incomplete and non-functional state**, which allows the rewrite to proceed without the need for strict backward compatibility.

During the transition:
- Existing **GDScript files will only be removed or deprecated once a corresponding C++ implementation exists**, either as:
    - a matching `.h` / `.cpp` pair, or
    - a clearly documented C++ module implementing the same logic.
- Until then, GDScript code may remain as a **reference implementation** or conceptual guide.
- Mixed-language coexistence is expected temporarily and is considered intentional.

The end goal is a **clean separation** between:
- a reusable C++ core system, and
- an optional Godot-based frontend.

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

Copyright 2025 SaintlyTea
Licensed under the Apache License, Version 2.0
