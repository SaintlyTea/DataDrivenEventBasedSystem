#include "Scripts/Helpers/Context/ContextBuilder.h"

#include "Scripts/Models/Events/CombatEvents/EventCombatBase.h"
#include "Scripts/Models/EventContext/Combat/EventCombatContext.h"

namespace
{
    // Most derived class first.
    std::shared_ptr<EventContext> make_for(const EventBase& event, const EventContext& parent)
    {
        if (dynamic_cast<const EventCombatBase*>(&event))
            return std::make_shared<EventCombatContext>(event.type, parent.trigger_source, std::vector<std::string>{});

        return std::make_shared<EventContext>(event.type, parent.trigger_source);
    }
}

std::shared_ptr<EventContext> ContextBuilder::build_for(
    const EventBase& event,
    const EventContext& parent,
    DerivationKind kind)
{
    auto ctx = make_for(event, parent);
    ctx->derive_from(parent, kind);
    return ctx;
}
