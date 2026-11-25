#include "Scripts/Models/Events/EventBase.h"

EventBase::EventBase(
    std::string pId,
    std::string pName,
    std::weak_ptr<IEventOwner> pSource,
    std::vector<std::string> pTriggers,
    std::string pConditionString,
    std::vector<std::string> pTags,
    std::string pType,
    std::string pFollowUpTrigger,
    int pPriority,
    std::string pInstanceId,
    std::string pAnnouncementTrigger)
    : conditions_(ExpressionStore::get_or_create(pConditionString)),
      id(pId),
      instance_id(pInstanceId.empty() ? pId : pInstanceId),
      name(pName),
      source(pSource),
      triggers(pTriggers),
      type(pType),
      follow_up_trigger(pFollowUpTrigger),
      announcement_trigger(pAnnouncementTrigger),
      tags(pTags),
      priority(pPriority)
{}

bool EventBase::execute(EventContext& ctx)
{
    if (ctx.is_event_cancelled(id, instance_id)) return false;
    if (!conditions_->evaluate(ctx)) return false;

    execute_(ctx);
    return true;
}

bool EventBase::evaluate_condition(EventContext& ctx) const
{
    return conditions_->evaluate(ctx);
}

void EventBase::set_new_conditions(std::shared_ptr<Expression> newExpression)
{
    conditions_ = newExpression;
}