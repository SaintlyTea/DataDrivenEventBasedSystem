#include "Scripts/States/ExpressionStore.h"
#include "Scripts/Models/Expression.h"

#include <unordered_map>
#include <regex>

namespace {
    // Function-local statics: initialized once, on first use (thread-safe since C++11).
    std::unordered_map<std::string, std::weak_ptr<Expression>> pool;
    bool normalize = true;
}

std::string ExpressionStore::norm_(const std::string& s) 
{
    if (!normalize) return s;
    // Remove all whitespace
    static const std::regex ws(R"REGEX(\s+)REGEX");
    return std::regex_replace(s, ws, "");
}

void ExpressionStore::set_normalize(bool enable) 
{
    normalize = enable;
}

std::shared_ptr<Expression> ExpressionStore::get_or_create(const std::string& src) 
{
    const std::string key = norm_(src);

    // If exists and still alive -> return existing
    if (auto it = pool.find(key); it != pool.end()) 
        if (auto existing = it->second.lock()) 
            return existing;

    // Create new and store weak ref
    auto created = std::make_shared<Expression>(src);
    pool[key] = created;
    return created;
}

void ExpressionStore::clear() 
{
    pool.clear();
}
