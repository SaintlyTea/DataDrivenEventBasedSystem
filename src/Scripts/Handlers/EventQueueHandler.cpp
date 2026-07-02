#include "Scripts/Handlers/EventQueueHandler.h"

void EventQueueHandler::process(EventOpQueue& queue, const EventContext& ctx)
{
    while (!queue.is_empty())
    {
        EventBase* event = queue.pop_next();
        event->execute(ctx);
    }
}