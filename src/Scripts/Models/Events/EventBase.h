#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Scripts/Models/Expression.h"
#include "Scripts/States/ExpressionStore.h"
#include "Scripts/Models/EventContext/EventContext.h"
#include "Scripts/Models/Entities/EventOwners/IEventOwner.h"

class EventBase
{
private:
    std::shared_ptr<Expression> conditions_;

public:
    const std::string id; // definition id
    const std::string instance_id; // defaults to id
    const std::string name;
    const std::weak_ptr<IEventOwner> source;
    const std::vector<std::string> triggers;
    const std::string type;
    const std::string follow_up_trigger;
    const std::string announcement_trigger;
    const std::vector<std::string> tags;
    const int priority {50}; // mid-level default

    EventBase(
        std::string pId,
        std::string pName,
        std::weak_ptr<IEventOwner> pSource,
        std::vector<std::string> pTriggers,
        std::string pConditionString,
        std::vector<std::string> pTags = {},
        std::string pType = "",
        std::string pFollowUpTrigger = "",
        int pPriority = 50,
        std::string pInstanceId = "",
        std::string pAnnouncementTrigger = "");

    virtual ~EventBase() = default;

    bool execute(EventContext& ctx);
    bool evaluate_condition(EventContext& ctx) const;
    void set_new_conditions(std::shared_ptr<Expression> newExpression);

    std::string get_name() const { return name; }

    virtual int compare_to(const EventBase&) const { return 0; }

protected:
    virtual void execute_(EventContext& ctx)
    {
        (void)ctx;
    }
};