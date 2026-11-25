#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "Scripts/Models/Events/EventBase.h"

class EventStore
{
private:
    std::unordered_map<std::string, std::vector<std::weak_ptr<EventBase>>>   owned_events_;
    std::unordered_map<std::string, std::vector<std::shared_ptr<EventBase>>> permanent_events_;

    static EventStore& instance_();

public:
    static void register_event(std::shared_ptr<EventBase> event);
    static void register_permanent_event(std::shared_ptr<EventBase> event);

    static void unregister_event(std::shared_ptr<EventBase> event);
    static void unregister_permanent_event(std::shared_ptr<EventBase> event);

    static std::vector<std::shared_ptr<EventBase>> get_events_for_trigger(const std::string& trigger);

    static void clear();
};