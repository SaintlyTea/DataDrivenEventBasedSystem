#include "Scripts/Models/EventContext/EventContext.h"

EventContext::EventContext(std::string event_type, std::string trigger_source)
    : event_type(event_type), trigger_source(trigger_source)
{}

void EventContext::set_payload(const std::string& key, std::unique_ptr<ContextPayload> payload)
{
    if (!payload) return;

    auto it = payloads_.find(key);
    if (it != payloads_.end())
    {
        // Key exists - merge incoming into existing
        auto merged = it->second->merge_with(*payload);
        if (merged)
            it->second = std::move(merged);
        // If merge returned nullptr (incompatible types), keep existing unchanged
    }
    else
    {
        payloads_[key] = std::move(payload);
    }
}

ContextPayload* EventContext::get_payload(const std::string& key) const
{
    auto it = payloads_.find(key);
    if (it != payloads_.end())
        return it->second.get();
    return nullptr;
}

bool EventContext::has_payload(const std::string& key) const
{
    return payloads_.find(key) != payloads_.end();
}