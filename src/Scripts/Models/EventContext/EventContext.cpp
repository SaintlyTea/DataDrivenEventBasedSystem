#include <cassert>

#include "Scripts/Models/EventContext/EventContext.h"
#include "Scripts/Models/ContextPayload/Utility/CancellationPayload.h"
#include "Scripts/Models/ContextPayload/Utility/BanPayload.h"

namespace
{
    CancellationPayload& cancellations(const EventContext& ctx)
    {
        auto* payload = dynamic_cast<CancellationPayload*>(ctx.get_payload(CancellationPayload::KEY));
        assert(payload != nullptr && "Every context is created with a CancellationPayload");
        return *payload;
    }

    BanPayload& bans(const EventContext& ctx)
    {
        auto* payload = dynamic_cast<BanPayload*>(ctx.get_payload(BanPayload::KEY));
        assert(payload != nullptr && "Every context is created with a BanPayload");
        return *payload;
    }
}

EventContext::EventContext(std::string event_type, std::string trigger_source)
    : event_type(std::move(event_type)),
      trigger_source(std::move(trigger_source)),
      depth_tracker(std::make_shared<EventDepthTracker>())
{
    payloads_[CancellationPayload::KEY] = std::make_shared<CancellationPayload>();
    payloads_[BanPayload::KEY] = std::make_shared<BanPayload>();
}

std::unique_ptr<EventContext> EventContext::create_derived(
    std::string new_event_type,
    std::string new_trigger_source,
    DerivationKind kind) const
{
    auto ctx = std::make_unique<EventContext>(std::move(new_event_type), std::move(new_trigger_source));
    ctx->derive_from(*this, kind);
    return ctx;
}

void EventContext::derive_from(const EventContext& parent, DerivationKind kind)
{
    depth_tracker = parent.depth_tracker;

    // Assign directly rather than through set_payload: this context's own fresh payloads
    // (e.g. its empty CancellationPayload) must be replaced, not merged into.
    for (const auto& [key, payload] : parent.payloads_)
    {
        switch (payload->propagation())
        {
            case PropagationMode::None:
                break;
            case PropagationMode::SharedReference:
                payloads_[key] = payload;
                break;
            case PropagationMode::Gather:
                if (kind == DerivationKind::Gather)
                    payloads_[key] = payload;
                break;
            case PropagationMode::Copy:
            {
                std::shared_ptr<ContextPayload> copy = payload->clone();
                assert(copy != nullptr && "Copy-propagating payload returned nullptr from clone()");
                payloads_[key] = std::move(copy);
                break;
            }
        }
    }
}

void EventContext::set_payload(const std::string& key, std::shared_ptr<ContextPayload> payload)
{
    if (!payload) return;

    auto it = payloads_.find(key);
    if (it != payloads_.end())
        it->second->merge_from(*payload);
    else
        payloads_.emplace(key, std::move(payload));
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

void EventContext::cancel_instance(const std::string& instance_id, bool permanent)
{
    cancellations(*this).cancel_instance(instance_id, permanent);
}

void EventContext::cancel_definition(const std::string& definition_id, bool permanent)
{
    cancellations(*this).cancel_definition(definition_id, permanent);
}

bool EventContext::is_event_cancelled(const std::string& definition_id, const std::string& instance_id)
{
    return cancellations(*this).check_and_consume(definition_id, instance_id);
}

void EventContext::ban_definition(const std::string& definition_id)
{
    bans(*this).ban(definition_id);
}

bool EventContext::is_definition_banned(const std::string& definition_id) const
{
    return bans(*this).is_banned(definition_id);
}
