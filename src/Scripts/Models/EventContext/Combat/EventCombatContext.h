#pragma once
#include "Scripts/Models/EventContext/EventContext.h"
#include <string>
#include <vector>

class EventCombatContext : public EventContext {
    public:
        std::vector<std::string> targets{};

        EventCombatContext(std::string pEvent_type, 
            std::string pTrigger_source, 
            std::vector<std::string> pTargets)
            : EventContext(pEvent_type, pTrigger_source),
              targets(std::move(pTargets))
        {}
};