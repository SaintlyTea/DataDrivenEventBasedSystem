#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "Scripts/Models/ContextPayload/ContextPayload.h"

class EventContext
{
private:
    std::unordered_map<std::string, std::unique_ptr<ContextPayload>> payloads_;

public:
    const std::string event_type;
    const std::string trigger_source;

    EventContext(std::string event_type, std::string trigger_source);
    virtual ~EventContext() = default;

    void set_payload(const std::string& key, std::unique_ptr<ContextPayload> payload);
    ContextPayload* get_payload(const std::string& key) const;
    bool has_payload(const std::string& key) const;
};