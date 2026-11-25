#include "Scripts/Handlers/EventFiring.h"
#include "Scripts/Handlers/EventSorter.h"
#include "Scripts/Handlers/EventQueueHandler.h"
#include "Scripts/Helpers/Context/ContextBuilder.h"
#include "Scripts/States/EventStore.h"

namespace
{
    QueuedEvent make_entry(const std::shared_ptr<EventBase>& event, const std::shared_ptr<EventContext>& cause, DerivationKind kind)
    {
        QueuedEvent entry {};
        entry.event = event;
        entry.own_context = ContextBuilder::build_for(*event, *cause, kind);
        entry.triggering_context = cause;
        return entry;
    }

    void run_sub_run(const std::shared_ptr<EventContext>& ctx, const std::string& trigger, DerivationKind kind)
    {
        EventQueue queue;
        for (const auto& e : EventFiring::get_sorted_events_for_trigger(trigger, *ctx))
            queue.push_normal_event(make_entry(e, ctx, kind));

        EventQueueHandler handler;
        handler.process(queue, ctx);
    }
}

std::vector<std::shared_ptr<EventBase>> EventFiring::get_sorted_events_for_trigger(
    const std::string& trigger,
    const EventContext& ctx)
{
    return EventSorter::sort(EventStore::get_events_for_trigger(trigger), ctx);
}

void EventFiring::queue_trigger(
    EventQueue& queue,
    const std::shared_ptr<EventContext>& cause,
    const std::string& trigger)
{
    EventQueue sorted;
    for (const auto& e : get_sorted_events_for_trigger(trigger, *cause))
        sorted.push_normal_event(make_entry(e, cause, DerivationKind::Normal));

    // Reverses into LIFO, so the first sorted event pops first.
    queue.enqueue_barrier_events(sorted);
}

void EventFiring::fire_trigger(
    const std::shared_ptr<EventContext>& ctx,
    const std::string& trigger)
{
    run_sub_run(ctx, trigger, DerivationKind::Normal);
}

void EventFiring::gather_trigger(
    const std::shared_ptr<EventContext>& ctx,
    const std::string& trigger)
{
    run_sub_run(ctx, trigger, DerivationKind::Gather);
}
