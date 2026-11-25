#include <algorithm>

#include "Scripts/States/EventStore.h"

EventStore& EventStore::instance_()
{
    static EventStore instance;
    return instance;
}

void EventStore::register_event(std::shared_ptr<EventBase> event)
{
    auto& store = instance_();
    for (const auto& trigger : event->triggers)
        store.owned_events_[trigger].push_back(event);
}

void EventStore::register_permanent_event(std::shared_ptr<EventBase> event)
{
    auto& store = instance_();
    for (const auto& trigger : event->triggers)
        store.permanent_events_[trigger].push_back(event);
}

void EventStore::unregister_event(std::shared_ptr<EventBase> event)
{
    auto& store = instance_();
    for (const auto& trigger : event->triggers)
    {
        auto it = store.owned_events_.find(trigger);
        if (it == store.owned_events_.end()) continue;

        auto& vec = it->second;
        vec.erase(
            std::remove_if(vec.begin(), vec.end(),
                [&event](const std::weak_ptr<EventBase>& wp) {
                    auto locked = wp.lock();
                    return !locked || locked->instance_id == event->instance_id;
                }),
            vec.end()
        );
    }
}

void EventStore::unregister_permanent_event(std::shared_ptr<EventBase> event)
{
    auto& store = instance_();
    for (const auto& trigger : event->triggers)
    {
        auto it = store.permanent_events_.find(trigger);
        if (it == store.permanent_events_.end()) continue;

        auto& vec = it->second;
        vec.erase(
            std::remove_if(vec.begin(), vec.end(),
                [&event](const std::shared_ptr<EventBase>& sp) {
                    return sp->instance_id == event->instance_id;
                }),
            vec.end()
        );
    }
}

std::vector<std::shared_ptr<EventBase>> EventStore::get_events_for_trigger(const std::string& trigger)
{
    auto& store = instance_();
    std::vector<std::shared_ptr<EventBase>> result;

    auto owned_it = store.owned_events_.find(trigger);
    if (owned_it != store.owned_events_.end())
    {
        for (const auto& event : owned_it->second)
        {
            if (auto locked = event.lock())
                result.push_back(locked);
        }
    }

    auto permanent_it = store.permanent_events_.find(trigger);
    if (permanent_it != store.permanent_events_.end())
    {
        for (const auto& event : permanent_it->second)
            result.push_back(event);
    }

    return result;
}

void EventStore::clear()
{
    auto& store = instance_();
    store.owned_events_.clear();
    store.permanent_events_.clear();
}