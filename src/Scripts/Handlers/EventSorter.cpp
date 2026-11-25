#include "Scripts/Handlers/EventSorter.h"
#include "Scripts/Models/Events/CombatEvents/EventCombatBase.h"
#include "Scripts/Models/EventContext/Combat/EventCombatContext.h"

#include <algorithm>

std::vector<std::shared_ptr<EventBase>> EventSorter::sort(
    std::vector<std::shared_ptr<EventBase>> events,
    const EventContext& ctx)
{
    std::stable_sort(events.begin(), events.end(),
        [&ctx](const std::shared_ptr<EventBase>& a, const std::shared_ptr<EventBase>& b)
        {
            // Level 1: Priority (higher wins)
            int prio_a = a->priority;
            int prio_b = b->priority;
            if (prio_a != prio_b) return prio_a > prio_b;

            // Level 2: Owner matches trigger source
            bool src_a = matches_source(a, ctx);
            bool src_b = matches_source(b, ctx);
            if (src_a != src_b) return src_a > src_b;

            // Level 3: Owner matches trigger target (skipped if no targets)
            bool tgt_a = matches_target(a, ctx);
            bool tgt_b = matches_target(b, ctx);
            if (tgt_a != tgt_b) return tgt_a > tgt_b;

            // Level 4: Event definition id
            if (a->id != b->id) return a->id < b->id;

            // Level 5: Type-specific compare_to (optional override)
            int cmp = a->compare_to(*b);
            if (cmp != 0) return cmp < 0;

            // Level 6: Owner definition id
            std::string def_a = get_owner_definition_id(a);
            std::string def_b = get_owner_definition_id(b);
            if (def_a != def_b) return def_a < def_b;

            // Level 7: Registration order within owner's list
            int reg_a = get_registration_order(a);
            int reg_b = get_registration_order(b);
            if (reg_a != reg_b) return reg_a < reg_b;

            // Level 8: Instance id (last resort)
            return get_owner_instance_id(a) < get_owner_instance_id(b);
        });

    return events;
}

bool EventSorter::matches_source(const std::shared_ptr<EventBase>& event, const EventContext& ctx)
{
    const std::string owner_id = get_owner_instance_id(event);
    return !owner_id.empty() && owner_id == ctx.trigger_source;
}

bool EventSorter::matches_target(const std::shared_ptr<EventBase>& event, const EventContext& ctx)
{
    const auto* combat_ctx = dynamic_cast<const EventCombatContext*>(&ctx);
    if (!combat_ctx || combat_ctx->targets.empty()) return false;

    const std::string owner_id = get_owner_instance_id(event);
    if (owner_id.empty()) return false;

    for (const auto& target : combat_ctx->targets)
        if (target == owner_id) return true;

    return false;
}

std::string EventSorter::get_owner_definition_id(const std::shared_ptr<EventBase>& event)
{
    auto* combat = dynamic_cast<EventCombatBase*>(event.get());
    if (combat)
    {
        if (auto owner = combat->owner.lock())
            return owner->get_definition_id();
    }

    if (auto source = event->source.lock())
        return source->get_definition_id();

    return "";
}

std::string EventSorter::get_owner_instance_id(const std::shared_ptr<EventBase>& event)
{
    auto* combat = dynamic_cast<EventCombatBase*>(event.get());
    if (combat)
    {
        if (auto owner = combat->owner.lock())
            return owner->get_instance_id();
    }

    if (auto source = event->source.lock())
        return source->get_instance_id();

    return "";
}

int EventSorter::get_registration_order(const std::shared_ptr<EventBase>& event)
{
    auto* combat = dynamic_cast<EventCombatBase*>(event.get());
    if (combat)
    {
        if (auto owner = combat->owner.lock())
            return owner->get_event_registration_order(event->instance_id);
    }

    if (auto source = event->source.lock())
        return source->get_event_registration_order(event->instance_id);

    return -1;
}