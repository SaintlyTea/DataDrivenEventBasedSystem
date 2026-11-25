#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Scripts/Models/Events/EventBase.h"
#include "Scripts/Models/EventContext/EventContext.h"

class EventSorter
{
public:
    static std::vector<std::shared_ptr<EventBase>> sort(
        std::vector<std::shared_ptr<EventBase>> events,
        const EventContext& ctx);

private:
    static bool matches_source(const std::shared_ptr<EventBase>& event, const EventContext& ctx);
    static bool matches_target(const std::shared_ptr<EventBase>& event, const EventContext& ctx);
    static std::string get_owner_definition_id(const std::shared_ptr<EventBase>& event);
    static std::string get_owner_instance_id(const std::shared_ptr<EventBase>& event);
    static int get_registration_order(const std::shared_ptr<EventBase>& event);
};
