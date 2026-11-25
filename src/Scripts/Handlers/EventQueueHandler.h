#pragma once

#include "Scripts/Models/EventOpQueue.h"
#include "Scripts/Models/EventContext/EventContext.h"

class EventQueueHandler
{
public:
    void process(EventOpQueue& queue, const EventContext& ctx);
};