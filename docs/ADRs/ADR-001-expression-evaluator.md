# ADR-001: Lazy Postfix Expression Evaluator for Event Conditions

**Status:** Accepted

---

## Context

Events and effects need a way to define activation conditions that is readable, serializable, and composable. Hard-coding conditions as C++ `if` statements would scatter logic across the codebase and make data-driven configuration impossible. Conditions need to be expressible as plain strings in JSON data files, support AND, OR, NOT, and XOR logic, and evaluate against live game state at runtime.

---

## Decision

Implement a lazy postfix expression evaluator. Conditions are written as infix strings using symbolic operators:

- `*` → AND
- `+` → OR
- `^` → XOR
- `-` → NOT

Atoms are type-value pairs: `{"TYPE","VALUE"}`. The type identifies which registered handler resolves the atom; the value is an opaque string interpreted entirely by that handler ((See this for parsing the values)[ADR-002-expression-atom-parsing]).

At load time, the string is tokenized and converted to postfix via the shunting-yard algorithm. At evaluation time, postfix tokens are processed into a stack of thunks — closures that are only called if needed. This gives short-circuit evaluation for free: an AND stops at the first false, an OR stops at the first true.

Atom results are memoized within a single evaluation cycle so identical checks are not repeated.

Parsed expressions are cached in `ExpressionStore` by normalized source string, so identical condition strings share one parsed structure across all events that use them.

The evaluation callback (`eval_atom`) is injected rather than hard-coded, keeping the expression system decoupled from any specific game state implementation.

---

## Consequences

- All systems share one condition language — combat, story, and AI events can all use the same format
- Conditions are plain strings, making them serializable and data-file friendly
- Short-circuit evaluation and memoization keep runtime cost low
- The expression system has no knowledge of what an atom's value means — that interpretation is fully delegated to whatever resolves `eval_atom`, keeping this layer stable even as the resolution strategy evolves
- Malformed expressions fail at load time rather than silently at runtime
- Complex nested conditions can be hard to read; tooling support may be needed later

---

## Alternatives Considered

- **Hard-coded C++ conditions** — not serializable, breaks data-driven design
- **AST object tree** — more introspectable but heavier to construct and unnecessary for boolean-only logic
- **Embedded scripting (Lua, etc.)** — too flexible at the cost of security, determinism, and runtime overhead