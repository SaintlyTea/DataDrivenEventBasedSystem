#pragma once
#include <memory>
#include <string>

#include "Scripts/Models/Expression.h"

class ExpressionStore {
public:
    static void set_normalize(bool enable);

    static std::shared_ptr<Expression> get_or_create(const std::string& src);

    static void clear();

private:
    static std::string norm_(const std::string& s);
};
