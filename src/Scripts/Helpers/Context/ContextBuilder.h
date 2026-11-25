#pragma once

#include <memory>

#include "Scripts/Models/Events/EventBase.h"
#include "Scripts/Models/EventContext/EventContext.h"

class ContextBuilder
{
public:
    static std::shared_ptr<EventContext> build_for(
        const EventBase& event,
        const EventContext& parent,
        DerivationKind kind = DerivationKind::Normal);
};
