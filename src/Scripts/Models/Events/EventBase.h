#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Scripts/Models/Expression.h"
#include "Scripts/States/ExpressionStore.h"
#include "Scripts/Models/EventContext/EventContext.h"

class EventBase
{
private:
    std::shared_ptr<Expression> conditions_;

public:
    const std::string id;
    const std::string name;
    const std::string source;
    const std::vector<std::string> tags;

    EventBase(
        std::string pId,
        std::string pName,
        std::string pSource,
        std::string pConditionString,
        std::vector<std::string> pTags = {});

    virtual ~EventBase() = default;

    void execute(const EventContext& ctx);
    bool evaluate_condition(const EventContext& ctx) const;
    void set_new_conditions(std::shared_ptr<Expression> newExpression);

    std::string get_name() const { return name; }

protected:
    virtual void execute_(const EventContext& ctx)
    {
        (void)ctx;
    }
};