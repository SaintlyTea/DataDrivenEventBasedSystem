# System Overview

This document describes the structure and responsibilities of the DataDrivenEventBasedSystem repository. It is meant to give orientation to anyone new to the codebase — where things live, why they exist, and how the core system works.

This is not a final specification. Several parts of the system are still evolving.

---

## Purpose

The goal is a data-driven, event-based foundation for turn-based games.

Gameplay behavior is expressed through events and data rather than tightly coupled logic. New abilities, mechanics, and interactions are added by defining new event types or data — not by modifying the core. The system handles execution flow; concrete behavior lives in the events themselves.

The design is generic enough to potentially support tick-based games, other genres, or non-game systems that benefit from deterministic event-driven logic.

---

## Directory Structure
src/
├── Scripts/
│   ├── Models/         Core data and logic models
│   ├── States/         Global state and caches
│   └── Tests/          Catch2 unit tests

The separation reflects: core logic (Models), supporting infrastructure (States), and verification (Tests).

---

## Core Components

### EventBase

The base class for all events. An event represents something that can happen in the system — dealing damage, applying a status, reacting to another event, advancing state.

Each event is responsible for:
- defining when it may execute (conditions, via Expression)
- enqueuing the work it produces (enqueue_)
- defining what happens when it executes (execute_)

New event types are added by subclassing EventBase. The rest of the system never needs to know what concrete types exist.

Currently the combat-specific base lives in `EventCombatBase`, which will later be split into a generic `EventBase` and a combat-specific subclass.

---

### Expression / ExpressionStore

`Expression` parses and evaluates infix boolean condition strings:
{"STAT","HP"} * -{"STATUS","STUNNED"}

Operators: `*` (AND), `+` (OR), `-` (NOT). Atoms are evaluated at runtime against live game state.

`ExpressionStore` caches expressions by source string so identical conditions share the same object. Expressions are freed automatically when no longer referenced.

---

### EventContext + ContextPayload

`EventContext` carries the information for an event chain — who caused it, relevant targets, and any typed payload data accumulated during processing.

Payloads are typed objects stored by key. When a payload is set on a key that already has one, they are merged. This allows multiple events to contribute to the same calculation (for example, several sources each adding flat damage bonuses) without knowing about each other.

Current concrete payloads:
- `DamageCalcData` — accumulates attack-side modifiers
- `DamageTakenData` — accumulates defense-side modifiers

---

### EventOpQueue

A dual-lane queue controlling execution order.

- **FIFO lane** — normal operations, processed in order
- **LIFO lane** — immediate operations, always processed before the FIFO lane

When an executing operation needs its follow-up work to complete before anything else in the queue, it pushes to the LIFO lane. This keeps complex event chains deterministic and predictable without recursion.

---

### Applier

Processes the queue. Pops operations one at a time, validates conditions, and calls execute on each. It has no knowledge of domain-specific behavior — that lives entirely in the events.

---

## Execution Flow

Using Firebolt as an example — a composite ability consisting of DealDamage followed by ApplyStatusEffect (Burning).

1. Player selects Firebolt and a target.
2. An `EventContext` is created.
3. A trigger `OnAbilityCastAnnounced` is emitted.
4. All events reacting to the announcement that meet their conditions are enqueued into the FIFO lane.
5. The Firebolt event itself is enqueued into the FIFO lane.
6. The Applier begins processing the queue.
7. When Firebolt executes, it expands into its child events and pushes them into the LIFO lane so they run consecutively before anything else.
8. DealDamage executes.
9. ApplyStatusEffect executes.
10. The Applier continues with whatever remains in the queue.

---

## Documentation Structure

- **Overview documents** — high-level explanations of structure and intent (this document)
- **ADRs** — records of why certain decisions were made
- **TODOs / Tickets** — track work in progress and architectural tasks
- **Concept documents** — planned; will cover system concepts once they stabilize