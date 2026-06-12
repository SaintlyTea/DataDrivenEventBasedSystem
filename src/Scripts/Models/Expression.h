#pragma once

#include <string>
#include <vector>
#include <functional>

#include "Scripts/Models/EventContext/EventContext.h"

class Expression
{
private:
    std::vector<std::string> postfix_{};
    const std::string src_;

    using EvalAtomFn = std::function<bool(const std::string&, const std::string&, const EventContext&)>;

    static EvalAtomFn eval_atom_;

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

    static void set_eval_atom(EvalAtomFn fn);
};