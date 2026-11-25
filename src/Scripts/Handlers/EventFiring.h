#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Scripts/Models/Events/EventBase.h"
#include "Scripts/Models/EventContext/EventContext.h"
#include "Scripts/Models/EventQueue/EventQueue.h"

class EventFiring
{
public:
    static std::vector<std::shared_ptr<EventBase>> get_sorted_events_for_trigger(
        const std::string& trigger,
        const EventContext& ctx);

    static void queue_trigger(
        EventQueue& queue,
        const std::shared_ptr<EventContext>& cause,
        const std::string& trigger);

    static void fire_trigger(
        const std::shared_ptr<EventContext>& ctx,
        const std::string& trigger);

    static void gather_trigger(
        const std::shared_ptr<EventContext>& ctx,
        const std::string& trigger);
};
