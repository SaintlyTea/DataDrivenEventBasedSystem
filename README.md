# DataDrivenEventBasedSystem

A data-driven, event-based backend for turn-based games, written in C++ and designed to be reusable across projects.

> This project is a work in progress. APIs and structures are subject to change, and a stable release does not exist yet.

---

## How It Works

Gameplay behavior is defined through events. Each event has a trigger, a set of conditions, and logic that executes when those conditions are met. When something happens in the game — an attack, a status effect, a turn starting — a trigger is emitted. Every event listening to that trigger evaluates its conditions and, if they pass, queues its work. A handler then runs the queue deterministically, with the ability to inject high-priority operations mid-run.

Event types are defined in code. Game objects such as characters and abilities are stored as JSON documents and loaded at runtime, using the existing event definitions to construct the objects with their configured data.

This is a backend only. It contains no frontend or rendering layer. An example of how to couple it with Godot via GDExtension may be provided later.

---

## Example: Firebolt

Firebolt is a composite ability made up of two events: DealDamage and ApplyStatusEffect (Burning).

1. The player casts Firebolt on a target.
2. A trigger `OnAbilityCastAnnounced` is emitted.
3. Any events reacting to that trigger and meeting their conditions are queued.
4. Firebolt itself is queued.
5. The processor begins running the queue.
6. When Firebolt executes, it expands into its child events and pushes them as high-priority operations, so they run consecutively before anything else.
7. DealDamage executes.
8. ApplyStatusEffect executes, applying Burning to the target.
9. The processor continues with whatever remains in the queue.

---

## Goal

To provide a reusable backend for turn-based games where gameplay logic lives in data and events rather than tightly coupled code. New abilities, mechanics, and interactions should be expressible by defining new event types or data files, not by modifying the core system.

The design is generic enough to potentially support other game types or non-game systems that benefit from deterministic event-driven logic.

---

## Non-Goals

- A complete game or playable product
- A replacement for existing game engines (Godot, Unity, Unreal, etc.)
- A polished editor or content pipeline
- Performance optimization at this stage — correctness and clarity come first

---

## Building

### Requirements

Install via [Chocolatey](https://chocolatey.org/):

```powershell
choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System' -y
choco install mingw -y
choco install ninja -y
```

### Configure and Build

```powershell
cmake --preset mingw-gcc-ninja
cmake --build build/mingw-gcc-ninja
```

### Run Tests

```powershell
cmake --build build/mingw-gcc-ninja --target tests
./build/mingw-gcc-ninja/tests.exe --success
```

[Catch2](https://github.com/catchorg/Catch2) is used as the testing framework and will be downloaded automatically by CMake during the build. No manual installation is needed.

---

## License

Licensed under the Apache License, Version 2.0