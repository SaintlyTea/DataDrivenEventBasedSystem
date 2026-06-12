#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <regex>

#include "Scripts/Models/Expression.h"
#include "Scripts/Models/EventContext/EventContext.h"

Expression::Expression(std::string conditionString) : src_(conditionString)
{
    setup();
}

std::string Expression::get_src_string() const
{
    return src_;
} 

Expression::EvalAtomFn Expression::eval_atom_ = nullptr;

void Expression::set_eval_atom(EvalAtomFn fn) {
    eval_atom_ = fn;
}

/* ========================= */

int Expression::precedence_(char op) const
{
        // OR < XOR < AND < NOT 
    switch (op) 
    {
        case '+': return 0;
        case '^': return 1;
        case '*': return 2;
        case '-': return 3; 
        default:  return -1;
    }
}

bool Expression::is_op_(char c) const 
{
    return c == '+' || c == '^' || c == '*' || c == '-';
}

std::vector<std::string> Expression::tokenize_() const
{
    std::vector<std::string> tokens;
    const int n = this->src_.size();

    for (int i = 0; i < n;)
    {
        const char ch = this->src_[i];

        // whitespace
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            ++i; 
            continue;
        }

        // atom block: {...}
        if (ch == '{') {
            int j = i + 1;
            int depth = 1;
            while (j < n && depth > 0) {
                if (this->src_[j] == '{') ++depth;
                else if (this->src_[j] == '}') --depth;
                ++j;
            }
            if (depth != 0) throw std::runtime_error("Unclosed atom starting at " + std::to_string(i));
            tokens.emplace_back(this->src_.substr(i, j - i));
            i = j;
            continue;
        }

        // operators / parensethes
        if (is_op_(ch) || ch == '(' || ch == ')') {
            tokens.emplace_back(1, ch);
            ++i;
            continue;
        }

        throw std::runtime_error(std::string("Unexpected char '") + ch + "' at " + std::to_string(i));
    }
    return tokens;
}

bool Expression::is_atom_(const std::string& tok) const
{
    return tok.size() >= 2 && tok.front() == '{' && tok.back() == '}'; 
}

std::vector<std::string> Expression::infix_to_postfix_(const std::vector<std::string>& tokens) const
{
    std::vector<std::string> result;
    std::vector<std::string> ops;
    bool expectValue = true;

    for (const auto& tok : tokens) {
        if (is_atom_(tok)) {
            result.push_back(tok);
            expectValue = false;
        }
        else if (tok == "(") {
            ops.push_back(tok);
            expectValue = true;
        }
        else if (tok == ")") {
            while (!ops.empty() && ops.back() != "(") {
                result.push_back(ops.back());
                ops.pop_back();
            }
            if (ops.empty()) throw std::runtime_error("Mismatched ')'"); // Change error type to something more fitting
            ops.pop_back(); // remove '('
            expectValue = false;
        }
        else if (tok.size() == 1 && is_op_(tok[0])) {
            const char op = tok[0];

            if (op == '-' && expectValue) {
                ops.push_back(tok);
                expectValue = true;
            } else {
                // binary op: pop while top has >= precedence
                while (!ops.empty() && ops.back() != "(") {
                    const char top = ops.back().size() == 1 ? ops.back()[0] : '\0';
                    if (!is_op_(top)) break;
                    if (precedence_(top) >= precedence_(op)) {
                        result.push_back(ops.back());
                        ops.pop_back();
                    } else break;
                }
                ops.push_back(tok);
                expectValue = true;
            }
        }
        else {
            throw std::runtime_error("Unknown token: " + tok);
        }
    }

    while (!ops.empty()) {
            auto top = ops.back();
            ops.pop_back();
            if (top == "(" || top == ")") throw std::runtime_error("Mismatched parentheses");
            result.push_back(std::move(top));
        }
        return result;
}

std::function<bool()> Expression::pop_(std::vector<std::function<bool()>>& stack) const
{
    if (stack.empty()) 
        throw std::runtime_error("Stack underflow while evaluating.");
    auto consumedFunc = std::move(stack.back());
    stack.pop_back();
    return consumedFunc;
}

std::pair<std::string, std::string> Expression::parse_atom_(const std::string& tok) const
{
    const std::string inner = tok.substr(1, tok.size() - 2);
    static const std::regex regExpresion {R"REGEX(^\s*"([^"]+)"\s*,\s*"([^"]+)"\s*$)REGEX"};
    std::smatch matchResults;

    if (!std::regex_match(inner, matchResults, regExpresion)) 
        throw std::runtime_error("Bad atom token: " + tok);
    return { matchResults[1].str(), matchResults[2].str() };
}


bool Expression::evaluate(const EventContext& ctx) const
{
    if (postfix_.empty()) 
        return true;

    std::unordered_map<std::string, bool> memo;
    std::vector<std::function<bool()>> stack;
    stack.reserve(postfix_.size());

    for (const auto& tok : postfix_) {
        if (is_atom_(tok)) {
            auto [ct, cv] = parse_atom_(tok);
            const std::string key = {ct + "\x1F" + cv};

            // thunk captures by value + references to memo/ctx/eval_atom
            stack.push_back([key, ct, cv, &memo, &ctx]() -> bool {
                if (auto it = memo.find(key); it != memo.end()) return it->second;
                bool res = eval_atom_(ct, cv, ctx); // TODO: Implement global or specific handler to evaluate conditions
                memo.emplace(key, res);
                return res;
            });
        }
        else if (tok == "*") { // AND (lazy)
            auto b = pop_(stack);
            auto a = pop_(stack);
            stack.push_back([a, b]() -> bool { return a() ? b() : false; });
        }
        else if (tok == "+") { // OR (lazy)
            auto b = pop_(stack);
            auto a = pop_(stack);
            stack.push_back([a, b]() -> bool { return a() ? true : b(); });
        }
        else if (tok == "^") { // XOR 
            auto b = pop_(stack);
            auto a = pop_(stack);
            stack.push_back([a, b]() -> bool { return a() != b(); });
        }
        else if (tok == "-") { // NOT
            auto a = pop_(stack);
            stack.push_back([a]() -> bool { return !a(); });
        }
        else {
            throw std::runtime_error("Unknown postfix token: " + tok);
        }
    }

    if (stack.size() != 1) {
        throw std::runtime_error("Invalid expression (stack size = " + std::to_string(stack.size()) + ")");
    }
    return stack[0]();
    
}

bool Expression::needs_setup() const 
{ 
    return postfix_.empty(); 
}

void Expression::setup()
{
    auto tokens = tokenize_();
    postfix_ = infix_to_postfix_(tokens);
}