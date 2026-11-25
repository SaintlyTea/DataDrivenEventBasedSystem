#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "Scripts/Models/ContextPayload/ContextPayload.h"
#include "Scripts/Models/EventContext/EventDepthTracker.h"

enum class DerivationKind
{
    Normal,
    Gather
};

class EventContext
{
private:
    std::unordered_map<std::string, std::shared_ptr<ContextPayload>> payloads_;

public:
    const std::string event_type;
    const std::string trigger_source;

    std::shared_ptr<EventDepthTracker> depth_tracker;

    EventContext(std::string event_type, std::string trigger_source);
    virtual ~EventContext() = default;

    EventContext(const EventContext&) = delete;
    EventContext& operator=(const EventContext&) = delete;

    EventContext(EventContext&&) = default;
    EventContext& operator=(EventContext&&) = default;

    std::unique_ptr<EventContext> create_derived(
        std::string new_event_type,
        std::string new_trigger_source,
        DerivationKind kind = DerivationKind::Normal) const;

    // No default for kind: defaults on virtual functions bind to the static type, not the override.
    virtual void derive_from(const EventContext& parent, DerivationKind kind);

    // Merges in place into an existing payload under key; an incompatible type is ignored.
    void set_payload(const std::string& key, std::shared_ptr<ContextPayload> payload);
    ContextPayload* get_payload(const std::string& key) const;
    bool has_payload(const std::string& key) const;

    void cancel_instance(const std::string& instance_id, bool permanent = false);
    void cancel_definition(const std::string& definition_id, bool permanent = false);

    bool is_event_cancelled(const std::string& definition_id, const std::string& instance_id);

    void ban_definition(const std::string& definition_id);
    bool is_definition_banned(const std::string& definition_id) const;
};
