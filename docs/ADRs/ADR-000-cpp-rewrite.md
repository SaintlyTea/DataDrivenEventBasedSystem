# ADR-000: Rewrite in C++ with Godot as Frontend Layer

**Status:** Accepted

---

## Context
The system was originally intended to be written in GDScript. The aim was to utilise the Godot Engine to make a game. But the goal changed and instead the base for a repurposable backbone to help developing games and perhaps other applications was decided upon. Due to Godot being rather limiting in that factor as it requires you to use Godot, the propostion was made, to switch to C++. On one hand this allows for a hopefully more optimised core. 

Additionally the expirience to gain and popularity of C++ was a great pull towards it.

---

## Decision
To rewrite the parts already implemented from Godot into C++20. Once completed, continue the development in C++20.
Godot will still be used over a library later on to create an API layer for the frontend.

---

## Consequences
- The backend can be built, run, and tested independently using CMake and Catch2
- The system is portable — it is not tied to any specific frontend or engine
- Connecting to Godot requires a GDExtension wrapper layer, which adds some integration work
- GDScript prototype serves as a design reference but is no longer part of the active codebase
- Requires learning C++ alongside building the system, which adds complexity but is intentional