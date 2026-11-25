#include "Scripts/Models/Events/EventBase.h"

EventBase::EventBase(
    std::string pId,
    std::string pName,
    std::string pSource,
    std::string pTrigger,
    std::string pConditionString,
    std::vector<std::string> pTags)
    : id(pId),
      name(pName),
      source(pSource),
      trigger(pTrigger),
      conditions_(ExpressionStore::get_or_create(pConditionString)),
      tags(pTags)
{}

void EventBase::execute(const EventContext& ctx)
{
    if (conditions_->evaluate(ctx))
        execute_(ctx);
}

bool EventBase::evaluate_condition(const EventContext& ctx) const
{
    return conditions_->evaluate(ctx);
}

void EventBase::set_new_conditions(std::shared_ptr<Expression> newExpression)
{
    conditions_ = newExpression;
}