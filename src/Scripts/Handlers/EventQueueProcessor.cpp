#include "Scripts/Handlers/EventQueueProcessor.h"

void EventQueueProcessor::process(EventOpQueue& queue, const EventContext& ctx)
{
    while (!queue.is_empty())
    {
        EventBase* event = queue.pop_next();
        event->execute(ctx);
    }
}