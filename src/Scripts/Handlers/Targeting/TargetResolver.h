#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Scripts/Constants/TargetingStrategy.h"
#include "Scripts/Models/EventContext/EventContext.h"

class TargetResolver
{
public:
    // Resolves targets for an event about to be queued.
    // explicit_targets is used only for the EXPLICIT strategy.
    static std::vector<std::string> resolve(
        TargetingStrategy strategy,
        const std::string& owner_id,
        const std::shared_ptr<EventContext>& triggering_context,
        const std::vector<std::string>& explicit_targets = {});
};