#pragma once

#include <memory>

#include "Scripts/Models/Events/EventBase.h"
#include "Scripts/Models/EventContext/EventContext.h"

struct QueuedEvent
{
    std::weak_ptr<EventBase> event {};
    std::shared_ptr<EventContext> own_context {nullptr};
    std::shared_ptr<EventContext> triggering_context {nullptr};
    bool has_announced {false};
};