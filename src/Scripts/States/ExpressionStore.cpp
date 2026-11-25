#include "Scripts/States/ExpressionStore.h"
#include "Scripts/Models/Expression.h"

#include <unordered_map>

namespace
{
    // Function-local statics: avoid the static initialization order problem.
    std::unordered_map<std::string, std::weak_ptr<Expression>>& pool()
    {
        static std::unordered_map<std::string, std::weak_ptr<Expression>> instance {};
        return instance;
    }

    bool& normalize()
    {
        static bool enabled {true};
        return enabled;
    }
}

// Whitespace inside quotes is part of an atom value and is kept.
std::string ExpressionStore::norm_(const std::string& s)
{
    if (!normalize()) return s;

    std::string out;
    out.reserve(s.size());
    bool in_quotes = false;
    for (char c : s)
    {
        if (c == '"') in_quotes = !in_quotes;
        const bool whitespace = c == ' ' || c == '\t' || c == '\n' || c == '\r';
        if (in_quotes || !whitespace)
            out.push_back(c);
    }
    return out;
}

void ExpressionStore::set_normalize(bool enable)
{
    normalize() = enable;
}

std::shared_ptr<Expression> ExpressionStore::get_or_create(const std::string& src)
{
    const std::string key = norm_(src);

    if (auto it = pool().find(key); it != pool().end())
        if (auto existing = it->second.lock())
            return existing;

    auto created = std::make_shared<Expression>(src);
    pool()[key] = created;
    return created;
}

void ExpressionStore::clear()
{
    pool().clear();
}
