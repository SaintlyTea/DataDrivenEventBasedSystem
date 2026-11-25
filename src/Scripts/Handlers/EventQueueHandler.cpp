#include "Scripts/Handlers/EventQueueHandler.h"
#include "Scripts/Handlers/EventFiring.h"
#include "Scripts/Helpers/Context/ContextBuilder.h"
#include "Scripts/Models/Events/EventBase.h"

#include <cassert>

namespace
{
    struct DepthGuard
    {
        EventDepthTracker& tracker;
        bool allowed {false};

        explicit DepthGuard(EventDepthTracker& t) : tracker(t)
        {
            allowed = tracker.enter_sub_handler();
        }

        ~DepthGuard()
        {
            if (allowed)
                tracker.exit_sub_handler();
        }

        DepthGuard(const DepthGuard&) = delete;
        DepthGuard& operator=(const DepthGuard&) = delete;
    };
}

void EventQueueHandler::process(EventQueue& queue, const std::shared_ptr<EventContext>& ctx_ptr)
{
    assert(ctx_ptr != nullptr && "process needs a context");
    EventContext& ctx = *ctx_ptr;

    DepthGuard guard(*ctx.depth_tracker);
    if (!guard.allowed)
    {
        // LOG: depth or visit tolerance limit hit, skipping this handler
        return;
    }

    while (!queue.is_empty())
    {
        QueuedEvent queued = queue.pop_next();
        const std::shared_ptr<EventBase> event = queued.event.lock();
        if (!event)
        {
            // LOG: event no longer exists, skipping
            continue;
        }
        const std::string& definition_id = event->id;

        if (!may_execute_(definition_id, ctx))
            continue;

        if (!queued.own_context)
        {
            queued.own_context = ContextBuilder::build_for(*event, ctx);
            queued.triggering_context = ctx_ptr;
        }
        const std::shared_ptr<EventContext>& own_ctx = queued.own_context;

        if (!event->announcement_trigger.empty() && !queued.has_announced)
        {
            if (!may_announce_(definition_id, ctx))
                continue;

            ++announcements_[definition_id];
            queued.has_announced = true;
            queue.push_barrier_event(queued);
            EventFiring::queue_trigger(queue, own_ctx, event->announcement_trigger);
            continue;
        }

        // Read before executing: a ban the event's own sub-runs issue doesn't charge this run.
        const bool banned_elsewhere = ctx.is_definition_banned(definition_id);

        if (!event->execute(*own_ctx))
            continue;

        ++executions_[definition_id];
        if (banned_elsewhere)
            ++ban_runs_[definition_id];

        if (!event->follow_up_trigger.empty())
            EventFiring::queue_trigger(queue, own_ctx, event->follow_up_trigger);
    }
}

bool EventQueueHandler::may_execute_(const std::string& definition_id, EventContext& ctx)
{
    if (banned_here_.contains(definition_id))
        return false;

    auto executed = executions_.find(definition_id);
    if (executed != executions_.end() && executed->second >= ctx.depth_tracker->width_tolerance)
    {
        // LOG: width limit hit at this level, banning definition_id
        ban_here_(definition_id, ctx);
        return false;
    }

    if (ctx.is_definition_banned(definition_id))
    {
        auto runs = ban_runs_.find(definition_id);
        const int used = (runs != ban_runs_.end()) ? runs->second : 0;
        if (used >= ctx.depth_tracker->ban_allowance)
            return false;
    }

    return true;
}

bool EventQueueHandler::may_announce_(const std::string& definition_id, EventContext& ctx)
{
    auto announced = announcements_.find(definition_id);
    if (announced != announcements_.end() && announced->second >= ctx.depth_tracker->width_tolerance)
    {
        // LOG: announcement limit hit at this level, banning definition_id
        ban_here_(definition_id, ctx);
        return false;
    }
    return true;
}

void EventQueueHandler::ban_here_(const std::string& definition_id, EventContext& ctx)
{
    ctx.ban_definition(definition_id);
    banned_here_.insert(definition_id);
}
