#pragma once

#include <string>
#include <vector>

#include "Scripts/Models/Events/EventBase.h"
#include "Scripts/Models/EventContext/Combat/EventCombatContext.h"

class EventCombatBase : public EventBase
{
public:
    bool enabled_stacks{false};
    int max_stacks{-1};
    int current_stacks{0};
    bool persists_combat{true};
    bool resetable{false};
    int duration_turns_left{0};
    int cd_turns_left{0};
    int cd_base{0};

    EventCombatBase(
        std::string pId,
        std::string pName,
        std::string pSource,
        std::string pTrigger,
        std::string pConditionString,
        std::vector<std::string> pTags = {},
        bool pEnableStacks = false,
        int pMaxStacks = -1,
        int pCurrentStacks = 0,
        bool pPersistsCombat = true,
        bool pResetable = false,
        int pDurationTurnsLeft = -1,
        int pCdTurnsLeft = 0,
        int pCdBase = 0);

    virtual ~EventCombatBase() = default;

protected:
    void execute_(const EventContext& ctx) override;
    virtual void execute_combat_(const EventCombatContext& ctx) {}
};