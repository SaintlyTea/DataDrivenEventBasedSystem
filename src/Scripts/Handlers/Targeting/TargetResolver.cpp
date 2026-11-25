#include "Scripts/Handlers/Targeting/TargetResolver.h"
#include "Scripts/Models/EventContext/Combat/EventCombatContext.h"

std::vector<std::string> TargetResolver::resolve(
    TargetingStrategy strategy,
    const std::string& owner_id,
    const std::shared_ptr<EventContext>& triggering_context,
    const std::vector<std::string>& explicit_targets)
{
    switch (strategy)
    {
        case TargetingStrategy::EXPLICIT:
            return explicit_targets;

        case TargetingStrategy::SELF:
            return { owner_id };

        case TargetingStrategy::INHERIT:
        {
            auto combat_ctx = std::dynamic_pointer_cast<EventCombatContext>(triggering_context);
            if (combat_ctx)
                return combat_ctx->targets;
            return {};
        }
    }
    return {};
}