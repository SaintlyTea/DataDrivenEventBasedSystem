# MyExpression.gd
class_name MyExpression
extends Resource

@export var src: String = ""   # original expression string

var _postfix: Array[String] = []
const _PREC := { '+':0, '^':1, '*':2, '-':3 }  # OR < XOR < AND < NOT

func _init(expr: String = "") -> void:
	if expr != "":
		setup(expr)

func needs_setup()->bool:
	return _postfix == []

func setup(expr: String) -> void:
	src = expr
	var tokens := _tokenize(expr)
	_postfix = _infix_to_postfix(tokens)

# ---------- Tokenizer ----------
func _tokenize(expr: String) -> Array[String]:
	var tokens: Array[String] = []
	var i := 0
	var n := expr.length()
	while i < n:
		var ch := expr[i]
		# whitespace
		if ch == ' ' or ch == '\t' or ch == '\n' or ch == '\r':
			i += 1
			continue
		# atom block: {"TYPE","VALUE"} (no nested braces expected)
		if ch == '{':
			var j := i + 1
			var depth := 1
			while j < n and depth > 0:
				if expr[j] == '{':
					depth += 1
				elif expr[j] == '}':
					depth -= 1
				j += 1
			if depth != 0:
				push_error("Unclosed atom starting at %d" % i)
				return []
			tokens.append(expr.substr(i, j - i))
			i = j
			continue
		# operators / parens
		if ch == '+' or ch == '*' or ch == '^' or ch == '-' or ch == '(' or ch == ')':
			tokens.append(ch)
			i += 1
			continue
		# anything else is unexpected
		push_error("Unexpected char '%s' at %d" % [ch, i])
		i += 1
	return tokens

# ---------- Atom parse ----------
func _is_atom(tok: String) -> bool:
	return tok.begins_with("{") and tok.ends_with("}")

func _parse_atom(tok: String) -> Array[String]:
	# {"TYPE","VALUE"} -> [TYPE, VALUE]
	var inner := tok.substr(1, tok.length() - 2)
	var reg := RegEx.new()
	reg.compile('^\\s*"([^"]+)"\\s*,\\s*"([^"]+)"\\s*$')
	var m := reg.search(inner)
	if m == null:
		push_error("Bad atom token: %s" % tok)
		return ["",""]
	return [m.get_string(1), m.get_string(2)]

# ---------- Infix => Postfix (shunting-yard), unary '-' handled ----------
func _infix_to_postfix(tokens: Array[String]) -> Array[String]:
	var out: Array[String] = []
	var ops: Array[String] = []
	var expect_value := true  # at start after '(' after op => a value or '(' or unary '-' is expected

	for tok in tokens:
		if _is_atom(tok):
			out.append(tok)
			expect_value = false
		elif tok == "(":
			ops.append(tok)
			expect_value = true
		elif tok == ")":
			while ops.size() > 0 and ops.back() != "(":
				out.append(ops.pop_back())
			if ops.size() == 0:
				push_error("Mismatched ')'")
				return []
			ops.pop_back() # remove '('
			expect_value = false
		elif _PREC.has(tok):
			if tok == "-" and expect_value:
				# unary NOT: highest precedence, right-assoc; just push
				ops.append(tok)
				expect_value = true
			else:
				# binary op: pop while top has >= precedence
				while ops.size() > 0 and ops.back() != "(" and _PREC.get(ops.back(), -1) >= _PREC[tok]:
					out.append(ops.pop_back())
				ops.append(tok)
				expect_value = true
		else:
			push_error("Unknown token: %s" % tok)
			return []

	while ops.size() > 0:
		var top :String= ops.pop_back()
		if top == "(" or top == ")":
			push_error("Mismatched parentheses")
			return []
		out.append(top)
	return out

# ---------- Lazy operators (short-circuit) ----------
static func _lazy_and(a: Callable, b: Callable) -> Callable:
	return func() -> bool:
		var av :bool= a.call()
		if not av:
			return false
		return b.call()

static func _lazy_or(a: Callable, b: Callable) -> Callable:
	return func() -> bool:
		var av :bool= a.call()
		if av:
			return true
		return b.call()

static func _lazy_xor(a: Callable, b: Callable) -> Callable:
	return func() -> bool:
		return a.call() != b.call()  # XOR has no natural short-circuit

static func _lazy_not(a: Callable) -> Callable:
	return func() -> bool:
		return not a.call()

# ---------- Evaluate with per-call memo & short-circuit ----------
func evaluate(ctx: EventContext) -> bool:
	# Build a small stack of thunks for this evaluation.
	# Each atom thunk consults a memo dict so repeated atoms are O(1).
	var memo := {}
	var stack: Array[Callable] = []

	for tok in _postfix:
		if _is_atom(tok):
			var p := _parse_atom(tok)
			var ct := p[0]
			var cv := p[1]
			var key := ct + "\\x1F" + cv  # nonprintable separator to avoid collisions
			var atom_fn := func() -> bool:
				if memo.has(key):
					return bool(memo[key])
				var res := GameMaster.state.eval(ct, cv, ctx)
				memo[key] = res
				return res
			stack.append(atom_fn)
		elif tok == "*":  # AND
			var b: Callable = stack.pop_back()
			var a: Callable = stack.pop_back()
			stack.append(_lazy_and(a, b))
		elif tok == "+":  # OR
			var b: Callable = stack.pop_back()
			var a:Callable = stack.pop_back()
			stack.append(_lazy_or(a, b))
		elif tok == "^":  # XOR
			var b: Callable = stack.pop_back()
			var a: Callable = stack.pop_back()
			stack.append(_lazy_xor(a, b))
		elif tok == "-":  # NOT (unary)
			var a: Callable = stack.pop_back()
			stack.append(_lazy_not(a))
		else:
			push_error("Unknown postfix token: %s" % tok)
			return false

	if stack.size() != 1:
		push_error("Invalid expression (stack size = %d)" % stack.size())
		return false

	return bool(stack[0].call())
