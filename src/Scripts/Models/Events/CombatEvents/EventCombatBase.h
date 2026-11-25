#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Scripts/Models/Events/EventBase.h"
#include "Scripts/Models/EventContext/Combat/EventCombatContext.h"
#include "Scripts/Models/Entities/EventOwners/IEventOwner.h"
#include "Scripts/Constants/TargetingStrategy.h"

class EventCombatBase : public EventBase
{
public:
    std::weak_ptr<IEventOwner> owner;
    TargetingStrategy targeting_strategy;
    bool enabled_stacks{false};
    int max_stacks{-1};
    int current_stacks{0};
    bool persists_combat{true};
    bool resetable{false};
    int duration_turns_left{-1}; // -1: no duration set
    int cd_turns_left{0};
    int cd_base{0};

    EventCombatBase(
        std::string pId,
        std::string pName,
        std::weak_ptr<IEventOwner> pSource,
        std::weak_ptr<IEventOwner> pOwner,
        std::vector<std::string> pTrigger,
        std::string pConditionString,
        TargetingStrategy pTargetingStrategy = TargetingStrategy::EXPLICIT,
        std::vector<std::string> pTags = {},
        std::string pType = "",
        std::string pFollowUpTrigger = "",
        int pPriority = 50,
        bool pEnableStacks = false,
        int pMaxStacks = -1,
        int pCurrentStacks = 0,
        bool pPersistsCombat = true,
        bool pResetable = false,
        int pDurationTurnsLeft = -1,
        int pCdTurnsLeft = 0,
        int pCdBase = 0,
        std::string pInstanceId = "",
        std::string pAnnouncementTrigger = "");

    virtual ~EventCombatBase() = default;

protected:
    void execute_(EventContext& ctx) override;
    virtual void execute_combat_(EventCombatContext&) {}
};