# DataDrivenEventBasedSystem

A work-in-progress **data-driven, event-based gameplay system**, primarily intended as a foundation for **turn-based games**.

This repository contains the core system logic written in **C++**, with **Godot (GDExtension)** planned as the frontend layer. It is **not a complete game** and **not a full engine**.

---

## Work in Progress

This project is actively under development.

- The current codebase is **incomplete and unstable**
- APIs, naming, and internal structures are subject to change
- Some parts are experimental or partially implemented
- A stable and fully documented release does not exist yet

If you are looking for a production-ready library: this is not it (yet).

---

## Goal

The main goal is to provide a reusable base for games where gameplay logic is expressed through **data and events** instead of hard-coded, tightly coupled systems.

The system aims to make it easier to:
- Add or change abilities, events, characters, and interactions by editing data files
- Keep complex gameplay logic maintainable as it grows
- Build gameplay features that remain testable and understandable

The design is intended to be generic enough to support multiple game types, with turn-based gameplay as the primary target.

---

## Non-Goals

This project does **not** aim to:
- Be a complete game or playable product
- Replace existing game engines (Godot, Unity, Unreal, etc.)
- Provide a polished editor or production-level content workflows
- Guarantee performance optimizations at this stage — correctness and clarity come first

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

Dependencies (currently only [Catch2](https://github.com/catchorg/Catch2)) are fetched automatically by CMake via `FetchContent` — no manual installation needed.

---

## Language and Architecture

The system is written in **C++20** with **Godot (GDExtension)** planned as the rendering and input frontend. The C++ backend is designed to have zero Godot dependencies, keeping the core logic portable and independently testable.

Game and ability data will eventually be loaded from **JSON documents**, making it possible to define characters, abilities, and events without modifying code.

---

## Documentation

See [`ARCHITECTURE.md`](ARCHITECTURE.md) for an overview of the core systems and how they relate.

Design decisions are documented incrementally as the system stabilizes. A more formal reference will be added once the core is stable enough that it won't become outdated weekly.

---

## License

Licensed under the Apache License, Version 2.0