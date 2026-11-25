#include "Scripts/Models/Events/CombatEvents/EventCombatBase.h"
#include "Scripts/Models/EventContext/EventContext.h"

EventCombatBase::EventCombatBase(
    std::string pId,
    std::string pName,
    std::string pSource,
    std::string pTrigger,
    std::string pConditionString,
    std::vector<std::string> pTags,
    bool pEnableStacks,
    int pMaxStacks,
    int pCurrentStacks,
    bool pPersistsCombat,
    bool pResetable,
    int pDurationTurnsLeft,
    int pCdTurnsLeft,
    int pCdBase)
    : EventBase(pId, pName, pSource, pTrigger, pConditionString, pTags),
      enabled_stacks(pEnableStacks),
      max_stacks(pMaxStacks),
      current_stacks(pCurrentStacks),
      persists_combat(pPersistsCombat),
      resetable(pResetable),
      duration_turns_left(pDurationTurnsLeft),
      cd_turns_left(pCdTurnsLeft),
      cd_base(pCdBase)
{}

void EventCombatBase::execute_(const EventContext& ctx)
{
    const EventCombatContext* combat_ctx = dynamic_cast<const EventCombatContext*>(&ctx);
    if (combat_ctx)
        execute_combat_(*combat_ctx);
}