#pragma once

#include "Scripts/Models/Events/CombatEvents/EventCombatBase.h"

class ApplyEffect : public EventCombatBase {
public:
    const std::string status_id;

    ApplyEffect(
        std::string pId,
        std::string pName,
        std::weak_ptr<IEventOwner> pSource,
        std::weak_ptr<IEventOwner> pOwner,
        std::vector<std::string> pTrigger,
        std::string pConditionString,
        std::string pStatusId)
        : EventCombatBase(pId, pName, pSource, pOwner, pTrigger, pConditionString),
          status_id(pStatusId)
    {}

protected:
    void execute_combat_(EventCombatContext& ctx) override;
};