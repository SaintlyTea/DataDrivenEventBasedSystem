#pragma once

#include <string>
#include <vector>
#include <functional>

#include "Scripts/Models/EventContext/EventContext.h"

class Expression
{
public:
    // Temporary fix/filler for global state manager or other solutions
    using EvalAtomFn = std::function<bool(const std::string&, const std::string&, const EventContext&)>;

private:
    std::vector<std::string> postfix_{};
    const std::string src_;
    // Function-local static: avoids the static initialization order problem.
    static EvalAtomFn& eval_atom_();

    std::function<bool()> pop_(std::vector<std::function<bool()>>& tokenList) const;
    bool is_atom_(const std::string& tok) const;
    std::pair<std::string, std::string> parse_atom_(const std::string& tok) const;
    int precedence_(char op) const;
    bool is_op_(char c) const;

    std::vector<std::string> tokenize_() const;

    std::vector<std::string> infix_to_postfix_(const std::vector<std::string>& tokens) const;

public:
    Expression(std::string conditionString);

    bool needs_setup() const;
    void setup();
    bool evaluate(const EventContext& ctx) const;
    std::string get_src_string() const;

    // Returns the previously set evaluator.
    static EvalAtomFn set_eval_atom(EvalAtomFn fn);
};