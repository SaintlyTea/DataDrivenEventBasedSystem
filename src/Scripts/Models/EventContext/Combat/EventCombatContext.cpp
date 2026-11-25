#include "Scripts/Models/EventContext/Combat/EventCombatContext.h"

EventCombatContext::EventCombatContext(
    std::string pEvent_type,
    std::string pTrigger_source,
    std::vector<std::string> pTargets)
    : EventContext(std::move(pEvent_type), std::move(pTrigger_source)),
      targets(std::move(pTargets))
{}

void EventCombatContext::derive_from(const EventContext& parent, DerivationKind kind)
{
    // targets is this level's only field. Whether a reaction inherits its parent's targets is
    // a targeting rule (TargetingStrategy::INHERIT), so it is not copied here.
    EventContext::derive_from(parent, kind);
}
