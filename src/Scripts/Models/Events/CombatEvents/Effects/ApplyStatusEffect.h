#pragma once

#include "Scripts/Models/Events/CombatEvents/EventCombatBase.h"
#include <iostream>

class ApplyEffect : public EventCombatBase {
public:
    const std::string status_id;

    ApplyEffect(
        std::string pId,
        std::string pName,
        std::string pSource,
        std::string pTrigger,
        std::string pConditionString,
        std::string pStatusId)
        : EventCombatBase(pId, pName, pSource, pTrigger, pConditionString),
          status_id(pStatusId)
    {}

protected:
    void execute_combat_(const EventCombatContext& ctx) override
    {
        std::cout << "[ApplyEffect] '" << get_name() 
                  << "' applying status '" << status_id 
                  << "' to " << ctx.targets.size() << " target(s)\n";
    }
};