#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <vector>
#include <algorithm>
#include <functional>

#include "Scripts/Models/Expression.h"

#include "Scripts/Handlers/EventQueueHandler.h"
#include "Scripts/Handlers/EventFiring.h"
#include "Scripts/Models/EventQueue/EventQueue.h"
#include "Scripts/Models/EventContext/EventContext.h"
#include "Scripts/Models/Events/EventBase.h"
#include "Scripts/States/EventStore.h"
#include "Scripts/Models/Entities/EventOwners/MockOwner.h"

// ===========================
// Test event stubs
// ===========================

class TrackingEvent : public EventBase
{
public:
    int id_num;
    std::vector<int>& log;

    TrackingEvent(
        int id,
        std::vector<int>& log,
        std::string follow_up = "",
        std::vector<std::string> triggers = {},
        int priority = 50)
        : EventBase(std::to_string(id), std::to_string(id), {}, triggers, "", {}, "", follow_up, priority),
          id_num(id), log(log)
    {}

protected:
    void execute_(EventContext&) override
    {
        log.push_back(id_num);
    }
};

class CancellingEvent : public EventBase
{
public:
    enum class Scope { Instance, Definition };

    std::string target_id;
    Scope scope;
    bool permanent;

    CancellingEvent(std::string id, std::string target, Scope pScope, bool pPermanent = false)
        : EventBase(id, id, {}, {}, "", {}),
          target_id(target),
          scope(pScope),
          permanent(pPermanent)
    {}

protected:
    void execute_(EventContext& ctx) override
    {
        if (scope == Scope::Instance)
            ctx.cancel_instance(target_id, permanent);
        else
            ctx.cancel_definition(target_id, permanent);
    }
};

class TrackingInstance : public EventBase
{
public:
    std::vector<std::string>& log;

    TrackingInstance(std::string def_id, std::string inst_id, std::vector<std::string>& pLog)
        : EventBase(def_id, def_id, {}, {}, "", {}, "", "", 50, inst_id), log(pLog)
    {}

protected:
    void execute_(EventContext&) override { log.push_back(instance_id); }
};

class SubHandlerEvent : public EventBase
{
public:
    std::vector<int>& log;
    int id_num;
    std::string sub_trigger;

    SubHandlerEvent(int id, std::vector<int>& log, std::string sub_trigger)
        : EventBase(std::to_string(id), std::to_string(id), {}, {},  "", {}),
          log(log), id_num(id), sub_trigger(sub_trigger)
    {}

protected:
    void execute_(EventContext& ctx) override
    {
        log.push_back(id_num);

        EventFiring::fire_trigger(ctx.create_derived("sub", ctx.trigger_source), sub_trigger);
    }
};

class DepthProbe : public EventBase
{
public:
    std::vector<int>& depths;
    bool& saw_visit_counts;

    DepthProbe(std::string id, std::vector<std::string> triggers, std::vector<int>& pDepths, bool& pSawVisitCounts)
        : EventBase(id, id, {}, triggers, "", {}), depths(pDepths), saw_visit_counts(pSawVisitCounts)
    {}

protected:
    void execute_(EventContext& ctx) override
    {
        depths.push_back(ctx.depth_tracker->current_depth);
        saw_visit_counts = !ctx.depth_tracker->depth_visit_counts.empty();
    }
};

static void clear_all()
{
    EventStore::clear();
}

static std::shared_ptr<EventContext> make_ctx()
{
    return std::make_shared<EventContext>("test", "unit_01");
}

// ===========================
// Basic execution
// ===========================
TEST_CASE("EventQueueHandler - single event executes", "[handler]")
{
    clear_all();
    std::vector<int> log;
    auto e = std::make_shared<TrackingEvent>(1, log);

    EventQueue q;
    q.push_normal_event(QueuedEvent{e});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log == std::vector<int>{1});
}

TEST_CASE("EventQueueHandler - multiple FIFO events execute in order", "[handler]")
{
    clear_all();
    std::vector<int> log;
    auto e1 = std::make_shared<TrackingEvent>(1, log);
    auto e2 = std::make_shared<TrackingEvent>(2, log);
    auto e3 = std::make_shared<TrackingEvent>(3, log);

    EventQueue q;
    q.push_normal_event(QueuedEvent{e1});
    q.push_normal_event(QueuedEvent{e2});
    q.push_normal_event(QueuedEvent{e3});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log == std::vector<int>{1, 2, 3});
}

TEST_CASE("EventQueueHandler - LIFO executes before FIFO", "[handler]")
{
    clear_all();
    std::vector<int> log;
    auto e1 = std::make_shared<TrackingEvent>(1, log);
    auto e2 = std::make_shared<TrackingEvent>(2, log);
    auto e3 = std::make_shared<TrackingEvent>(3, log);
    auto e4 = std::make_shared<TrackingEvent>(4, log);

    EventQueue q;
    q.push_normal_event(QueuedEvent{e1});
    q.push_normal_event(QueuedEvent{e2});
    q.push_barrier_event(QueuedEvent{e3});
    q.push_barrier_event(QueuedEvent{e4});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log == std::vector<int>{4, 3, 1, 2});
}

TEST_CASE("EventQueueHandler - empty queue processes without error", "[handler]")
{
    clear_all();
    EventQueue q;
    EventQueueHandler handler;
    REQUIRE_NOTHROW(handler.process(q, make_ctx()));
}

// ===========================
// Follow-up triggers
// ===========================
TEST_CASE("EventQueueHandler - follow-up trigger queues matching events", "[handler]")
{
    clear_all();
    std::vector<int> log;

    auto follow_up = std::make_shared<TrackingEvent>(2, log, "", std::vector<std::string>{"OnFollowUp"});
    EventStore::register_event(follow_up);

    auto e1 = std::make_shared<TrackingEvent>(1, log, "OnFollowUp");

    EventQueue q;
    q.push_normal_event(QueuedEvent{e1});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log == std::vector<int>{1, 2});
}

TEST_CASE("EventQueueHandler - follow-up trigger with no matching events completes", "[handler]")
{
    clear_all();
    std::vector<int> log;
    auto e1 = std::make_shared<TrackingEvent>(1, log, "OnFollowUp");

    EventQueue q;
    q.push_normal_event(QueuedEvent{e1});

    EventQueueHandler handler;
    REQUIRE_NOTHROW(handler.process(q, make_ctx()));
    REQUIRE(log == std::vector<int>{1});
}

// ===========================
// Cancellation
// ===========================
TEST_CASE("EventQueueHandler - cancelled event is skipped", "[handler]")
{
    clear_all();
    std::vector<int> log;

    auto e2 = std::make_shared<TrackingEvent>(2, log);
    auto canceller = std::make_shared<CancellingEvent>("canceller", "2", CancellingEvent::Scope::Instance);
    auto e1 = std::make_shared<TrackingEvent>(1, log);

    EventQueue q;
    q.push_normal_event(QueuedEvent{canceller});
    q.push_normal_event(QueuedEvent{e1});
    q.push_normal_event(QueuedEvent{e2});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log == std::vector<int>{1});
}

TEST_CASE("EventQueueHandler - permanent definition cancel blocks every instance", "[handler]")
{
    clear_all();
    std::vector<std::string> log;

    auto a = std::make_shared<TrackingInstance>("fireball", "fireball#a", log);
    auto b = std::make_shared<TrackingInstance>("fireball", "fireball#b", log);
    auto canceller = std::make_shared<CancellingEvent>("canceller", "fireball", CancellingEvent::Scope::Definition, true);

    EventQueue q;
    q.push_normal_event(QueuedEvent{canceller});
    q.push_normal_event(QueuedEvent{a});
    q.push_normal_event(QueuedEvent{b});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log.empty());
}

TEST_CASE("EventQueueHandler - one-time definition cancel blocks only the first occurrence", "[handler]")
{
    clear_all();
    std::vector<std::string> log;

    auto a = std::make_shared<TrackingInstance>("fireball", "fireball#a", log);
    auto b = std::make_shared<TrackingInstance>("fireball", "fireball#b", log);
    auto canceller = std::make_shared<CancellingEvent>("canceller", "fireball", CancellingEvent::Scope::Definition, false);

    EventQueue q;
    q.push_normal_event(QueuedEvent{canceller});
    q.push_normal_event(QueuedEvent{a});
    q.push_normal_event(QueuedEvent{b});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log == std::vector<std::string>{"fireball#b"});
}

TEST_CASE("EventQueueHandler - instance cancel blocks only that instance", "[handler]")
{
    clear_all();
    std::vector<std::string> log;

    auto a = std::make_shared<TrackingInstance>("fireball", "fireball#a", log);
    auto b = std::make_shared<TrackingInstance>("fireball", "fireball#b", log);
    auto canceller = std::make_shared<CancellingEvent>("canceller", "fireball#a", CancellingEvent::Scope::Instance, true);

    EventQueue q;
    q.push_normal_event(QueuedEvent{canceller});
    q.push_normal_event(QueuedEvent{a});
    q.push_normal_event(QueuedEvent{b});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log == std::vector<std::string>{"fireball#b"});
}

TEST_CASE("EventQueueHandler - cancel issued inside a sub-run blocks a pending event above it", "[handler]")
{
    clear_all();
    std::vector<std::string> log;

    class RegisteredCounter : public EventBase {
    public:
        RegisteredCounter() : EventBase("counter", "counter", {}, {"Counter"}, "", {}) {}
    protected:
        void execute_(EventContext& ctx) override { ctx.cancel_definition("fireball", true); }
    };
    auto registered = std::make_shared<RegisteredCounter>();
    EventStore::register_event(registered);

    class SpawnerEvent : public EventBase {
    public:
        SpawnerEvent() : EventBase("spawner", "spawner", {}, {}, "", {}) {}
    protected:
        void execute_(EventContext& ctx) override {
            EventFiring::fire_trigger(ctx.create_derived("Counter", "unit_02"), "Counter");
        }
    };

    auto spawner = std::make_shared<SpawnerEvent>();
    auto fireball = std::make_shared<TrackingInstance>("fireball", "fireball#a", log);

    EventQueue q;
    q.push_normal_event(QueuedEvent{spawner});
    q.push_normal_event(QueuedEvent{fireball});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log.empty());
}

// ===========================
// Depth tracking
// ===========================
TEST_CASE("EventQueueHandler - depth increments on sub-handler entry", "[handler]")
{
    clear_all();
    std::vector<int> log;
    std::vector<int> depths;
    bool saw_visit_counts = false;

    auto inner = std::make_shared<DepthProbe>("inner", std::vector<std::string>{"SubTrigger"}, depths, saw_visit_counts);
    EventStore::register_event(inner);

    auto e1 = std::make_shared<SubHandlerEvent>(1, log, "SubTrigger");
    auto outer = std::make_shared<DepthProbe>("outer", std::vector<std::string>{}, depths, saw_visit_counts);

    EventQueue q;
    q.push_normal_event(QueuedEvent{e1});
    q.push_normal_event(QueuedEvent{outer});

    auto ctx = make_ctx();
    REQUIRE(ctx->depth_tracker->current_depth == -1);

    EventQueueHandler handler;
    handler.process(q, ctx);

    REQUIRE(depths == std::vector<int>{1, 0});
    REQUIRE(ctx->depth_tracker->current_depth == -1);
}

TEST_CASE("EventQueueHandler - depth visit counts reset when fully unwound", "[handler]")
{
    clear_all();
    std::vector<int> log;
    std::vector<int> depths;
    bool saw_visit_counts = false;

    auto inner = std::make_shared<DepthProbe>("inner", std::vector<std::string>{"SubTrigger"}, depths, saw_visit_counts);
    EventStore::register_event(inner);
    auto e1 = std::make_shared<SubHandlerEvent>(1, log, "SubTrigger");

    EventQueue q;
    q.push_normal_event(QueuedEvent{e1});

    auto ctx = make_ctx();
    EventQueueHandler handler;
    handler.process(q, ctx);

    REQUIRE(saw_visit_counts);
    REQUIRE(ctx->depth_tracker->depth_visit_counts.empty());
}

// ===========================
// Depth limit
// ===========================
TEST_CASE("EventQueueHandler - depth limit stops execution gracefully", "[handler]")
{
    clear_all();
    std::vector<int> log;

    // Create a chain that keeps spawning sub-handlers
    // by registering an event under a trigger that itself creates a sub-handler
    class RecursingEvent : public EventBase {
    public:
        std::vector<int>& log;
        RecursingEvent(std::vector<int>& log)
            : EventBase("recurse", "recurse", {}, {"RecurseMe"}, "", {}), log(log) {}

    protected:
        void execute_(EventContext& ctx) override {
            log.push_back(ctx.depth_tracker->current_depth);
            EventFiring::fire_trigger(ctx.create_derived("sub", ctx.trigger_source), "RecurseMe");
        }
    };

    auto recurser = std::make_shared<RecursingEvent>(log);
    EventStore::register_event(recurser);

    EventQueue q;
    q.push_normal_event(QueuedEvent{recurser});

    auto ctx = make_ctx();
    ctx->depth_tracker->max_depth = 5; // low limit for testing

    EventQueueHandler handler;
    REQUIRE_NOTHROW(handler.process(q, ctx));
    REQUIRE(log == std::vector<int>{0, 1, 2, 3, 4, 5});
    REQUIRE(ctx->depth_tracker->current_depth == -1);
}

// ===========================
// Visit tolerance
// ===========================
TEST_CASE("EventQueueHandler - visit tolerance caps sub-handler entries at one depth", "[handler]")
{
    clear_all();
    std::vector<int> log;
    std::vector<int> depth_after_each_sub_run;

    auto leaf = std::make_shared<TrackingEvent>(7, log, "", std::vector<std::string>{"Leaf"});
    EventStore::register_event(leaf);

    class SpawnerEvent : public EventBase {
    public:
        std::vector<int>& depths;
        SpawnerEvent(std::vector<int>& d)
            : EventBase("spawner", "spawner", {}, {}, "", {}), depths(d) {}
    protected:
        void execute_(EventContext& ctx) override {
            for (int i = 0; i < 15; ++i) {
                EventFiring::fire_trigger(ctx.create_derived("sub", ctx.trigger_source), "Leaf");
                depths.push_back(ctx.depth_tracker->current_depth);
            }
        }
    };

    auto spawner = std::make_shared<SpawnerEvent>(depth_after_each_sub_run);
    EventQueue q;
    q.push_normal_event(QueuedEvent{spawner});

    auto ctx = make_ctx();
    ctx->depth_tracker->base_tolerance = 10;
    ctx->depth_tracker->min_tolerance  = 10;
    ctx->depth_tracker->scale_factor   = 0.0f; // flat tolerance for predictable test

    EventQueueHandler handler;
    REQUIRE_NOTHROW(handler.process(q, ctx));

    REQUIRE(log.size() == 10);
    REQUIRE(depth_after_each_sub_run == std::vector<int>(15, 0));
    REQUIRE(ctx->depth_tracker->current_depth == -1);
    REQUIRE(ctx->depth_tracker->depth_visit_counts.empty());
}

// ===========================
// Width limit
// ===========================
static std::shared_ptr<EventContext> make_ctx_with_width(int width)
{
    auto ctx = make_ctx();
    ctx->depth_tracker->width_tolerance = width;
    return ctx;
}

TEST_CASE("EventQueueHandler - width stops a self follow-up loop at the tolerance", "[handler][width]")
{
    clear_all();
    std::vector<int> log;

    auto looper = std::make_shared<TrackingEvent>(1, log, "LoopTrigger", std::vector<std::string>{"LoopTrigger"});
    EventStore::register_event(looper);

    EventQueue q;
    q.push_normal_event(QueuedEvent{looper});

    EventQueueHandler handler;
    REQUIRE_NOTHROW(handler.process(q, make_ctx_with_width(5)));
    REQUIRE(log.size() == 5);
}

TEST_CASE("EventQueueHandler - width budget is per definition id", "[handler][width]")
{
    clear_all();
    std::vector<int> log;

    auto a = std::make_shared<TrackingEvent>(1, log, "ToB", std::vector<std::string>{"ToA"});
    auto b = std::make_shared<TrackingEvent>(2, log, "ToA", std::vector<std::string>{"ToB"});
    EventStore::register_event(a);
    EventStore::register_event(b);

    EventQueue q;
    q.push_normal_event(QueuedEvent{a});

    EventQueueHandler handler;
    handler.process(q, make_ctx_with_width(3));

    REQUIRE(log == std::vector<int>{1, 2, 1, 2, 1, 2});
}

TEST_CASE("EventQueueHandler - events blocked at execute time use no width budget", "[handler][width]")
{
    clear_all();
    std::vector<std::string> log;

    auto canceller = std::make_shared<CancellingEvent>("canceller", "x", CancellingEvent::Scope::Definition, false);
    auto x1 = std::make_shared<TrackingInstance>("x", "x#1", log);
    auto x2 = std::make_shared<TrackingInstance>("x", "x#2", log);
    auto x3 = std::make_shared<TrackingInstance>("x", "x#3", log);

    EventQueue q;
    q.push_normal_event(QueuedEvent{canceller});
    q.push_normal_event(QueuedEvent{x1});
    q.push_normal_event(QueuedEvent{x2});
    q.push_normal_event(QueuedEvent{x3});

    EventQueueHandler handler;
    handler.process(q, make_ctx_with_width(2));

    REQUIRE(log == std::vector<std::string>{"x#2", "x#3"});
}

TEST_CASE("EventQueueHandler - a blocked event does not fire its follow-up", "[handler][width]")
{
    clear_all();
    std::vector<int> log;

    auto follow_up = std::make_shared<TrackingEvent>(2, log, "", std::vector<std::string>{"OnFollowUp"});
    EventStore::register_event(follow_up);

    auto canceller = std::make_shared<CancellingEvent>("canceller", "1", CancellingEvent::Scope::Definition, true);
    auto e1 = std::make_shared<TrackingEvent>(1, log, "OnFollowUp");

    EventQueue q;
    q.push_normal_event(QueuedEvent{canceller});
    q.push_normal_event(QueuedEvent{e1});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log.empty());
}

TEST_CASE("EventQueueHandler - width budget does not leak between handlers", "[handler][width]")
{
    clear_all();
    std::vector<int> log;

    auto leaf = std::make_shared<TrackingEvent>(7, log, "", std::vector<std::string>{"Leaf"});
    EventStore::register_event(leaf);

    class SpawnerEvent : public EventBase {
    public:
        SpawnerEvent() : EventBase("spawner", "spawner", {}, {}, "", {}) {}
    protected:
        void execute_(EventContext& ctx) override {
            for (int i = 0; i < 3; ++i) {
                EventFiring::fire_trigger(ctx.create_derived("sub", ctx.trigger_source), "Leaf");
            }
        }
    };

    auto spawner = std::make_shared<SpawnerEvent>();
    EventQueue q;
    q.push_normal_event(QueuedEvent{spawner});

    EventQueueHandler handler;
    handler.process(q, make_ctx_with_width(2));

    REQUIRE(log == std::vector<int>{7, 7, 7});
}

// ===========================
// Ban payload + per-handler allowance
// ===========================
class MultiSubRunEvent : public EventBase
{
public:
    using Runs = std::vector<std::vector<std::shared_ptr<EventBase>>>;
    Runs runs;

    explicit MultiSubRunEvent(Runs pRuns)
        : EventBase("multi", "multi", {}, {}, "", {}), runs(std::move(pRuns)) {}

protected:
    void execute_(EventContext& ctx) override
    {
        for (const auto& run : runs)
        {
            std::shared_ptr<EventContext> sub_ctx = ctx.create_derived("sub", "unit_01");
            EventQueue sub_q;
            for (const auto& e : run)
                sub_q.push_normal_event(QueuedEvent{e});
            EventQueueHandler sub_handler;
            sub_handler.process(sub_q, sub_ctx);
        }
    }
};

TEST_CASE("Ban - tripping width bans the definition id", "[handler][ban]")
{
    clear_all();
    std::vector<int> log;

    auto looper = std::make_shared<TrackingEvent>(1, log, "LoopTrigger", std::vector<std::string>{"LoopTrigger"});
    EventStore::register_event(looper);
    auto within_tolerance = std::make_shared<TrackingEvent>(2, log);

    EventQueue q;
    q.push_normal_event(QueuedEvent{looper});
    q.push_normal_event(QueuedEvent{within_tolerance});

    auto ctx = make_ctx_with_width(5);
    EventQueueHandler handler;
    handler.process(q, ctx);

    REQUIRE(log == std::vector<int>{1, 1, 1, 1, 1, 2});
    REQUIRE(ctx->is_definition_banned("1"));
    REQUIRE_FALSE(ctx->is_definition_banned("2"));
}

TEST_CASE("Ban - ban issued in a sub-handler is visible to the parent", "[handler][ban]")
{
    clear_all();
    std::vector<int> log;

    auto looper = std::make_shared<TrackingEvent>(1, log, "LoopTrigger", std::vector<std::string>{"LoopTrigger"});
    EventStore::register_event(looper);

    auto spawner = std::make_shared<MultiSubRunEvent>(MultiSubRunEvent::Runs{{looper}});
    EventQueue q;
    q.push_normal_event(QueuedEvent{spawner});

    auto ctx = make_ctx_with_width(5);
    EventQueueHandler handler;
    handler.process(q, ctx);

    REQUIRE(ctx->is_definition_banned("1"));
}

TEST_CASE("Ban - a banned id gets ban_allowance runs in another handler", "[handler][ban]")
{
    clear_all();
    std::vector<int> log;
    auto a = std::make_shared<TrackingEvent>(1, log);
    auto b = std::make_shared<TrackingEvent>(1, log);
    auto c = std::make_shared<TrackingEvent>(1, log);
    auto other = std::make_shared<TrackingEvent>(2, log);

    EventQueue q;
    q.push_normal_event(QueuedEvent{a});
    q.push_normal_event(QueuedEvent{b});
    q.push_normal_event(QueuedEvent{other});
    q.push_normal_event(QueuedEvent{c});

    auto ctx = make_ctx();
    ctx->ban_definition("1");   // banned elsewhere; this handler did not trip it

    EventQueueHandler handler;
    handler.process(q, ctx);

    REQUIRE(log == std::vector<int>{1, 2});   // N = 1 by default
}

TEST_CASE("Ban - each sibling handler gets its own allowance", "[handler][ban]")
{
    clear_all();
    std::vector<int> log;
    auto x1 = std::make_shared<TrackingEvent>(7, log);
    auto x2 = std::make_shared<TrackingEvent>(7, log);

    auto spawner = std::make_shared<MultiSubRunEvent>(MultiSubRunEvent::Runs{{x1, x2}, {x1, x2}});
    EventQueue q;
    q.push_normal_event(QueuedEvent{spawner});

    auto ctx = make_ctx();
    ctx->ban_definition("7");

    EventQueueHandler handler;
    handler.process(q, ctx);

    REQUIRE(log == std::vector<int>{7, 7});
}

TEST_CASE("Ban - allowance of zero blocks a banned id everywhere", "[handler][ban]")
{
    clear_all();
    std::vector<int> log;
    auto x = std::make_shared<TrackingEvent>(7, log);
    auto other = std::make_shared<TrackingEvent>(2, log);

    EventQueue q;
    q.push_normal_event(QueuedEvent{x});
    q.push_normal_event(QueuedEvent{other});

    auto ctx = make_ctx();
    ctx->depth_tracker->ban_allowance = 0;
    ctx->ban_definition("7");

    EventQueueHandler handler;
    handler.process(q, ctx);

    REQUIRE(log == std::vector<int>{2});
}

TEST_CASE("Ban - a looping step is cut off while the rest of the sequence still resolves", "[handler][ban]")
{
    clear_all();
    std::vector<int> log;

    auto a = std::make_shared<TrackingEvent>(1, log);
    auto b = std::make_shared<TrackingEvent>(2, log, "LoopB", std::vector<std::string>{"LoopB"});
    auto c = std::make_shared<TrackingEvent>(3, log);
    auto d = std::make_shared<TrackingEvent>(4, log);
    auto e = std::make_shared<TrackingEvent>(5, log);
    EventStore::register_event(b);

    // Run 1: A, B (loops via its follow-up), C, D. Run 2, a later sibling: B, B, E.
    auto sequence = std::make_shared<MultiSubRunEvent>(MultiSubRunEvent::Runs{{a, b, c, d}, {b, b, e}});
    EventQueue q;
    q.push_normal_event(QueuedEvent{sequence});

    auto ctx = make_ctx_with_width(5);
    EventQueueHandler handler;
    handler.process(q, ctx);

    // Run 1: B loops immediately (LIFO) until width bans it, then C, D.
    // Run 2: B gets its allowance of 1, the second B is skipped, then E.
    REQUIRE(log == std::vector<int>{1, 2, 2, 2, 2, 2, 3, 4, 2, 5});
    REQUIRE(ctx->is_definition_banned("2"));
}

// ===========================
// Follow-ups: lane and triggering context
// ===========================
TEST_CASE("Follow-up - runs before the next pending event", "[handler][follow-up]")
{
    clear_all();
    std::vector<int> log;

    auto listener = std::make_shared<TrackingEvent>(2, log, "", std::vector<std::string>{"OnFollowUp"});
    EventStore::register_event(listener);

    auto e1 = std::make_shared<TrackingEvent>(1, log, "OnFollowUp");
    auto e3 = std::make_shared<TrackingEvent>(3, log);

    EventQueue q;
    q.push_normal_event(QueuedEvent{e1});
    q.push_normal_event(QueuedEvent{e3});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log == std::vector<int>{1, 2, 3});
}

TEST_CASE("Follow-up - several listeners keep their sorted order", "[handler][follow-up]")
{
    clear_all();
    std::vector<int> log;

    // Registered low-priority first, so only the sort can put P ahead of Q.
    auto q_low  = std::make_shared<TrackingEvent>(40, log, "", std::vector<std::string>{"OnFollowUp"}, 40);
    auto p_high = std::make_shared<TrackingEvent>(60, log, "", std::vector<std::string>{"OnFollowUp"}, 60);
    EventStore::register_event(q_low);
    EventStore::register_event(p_high);

    auto e1 = std::make_shared<TrackingEvent>(1, log, "OnFollowUp");
    auto e3 = std::make_shared<TrackingEvent>(3, log);

    EventQueue q;
    q.push_normal_event(QueuedEvent{e1});
    q.push_normal_event(QueuedEvent{e3});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log == std::vector<int>{1, 60, 40, 3});
}

// ===========================
// Announcements (ADR-007)
// ===========================
class ScriptedEvent : public EventBase
{
public:
    std::vector<int>& log;
    int id_num;
    std::function<void(EventContext&)> on_run;

    ScriptedEvent(int id, std::vector<int>& pLog,
                  std::vector<std::string> triggers,
                  std::string announcement = "",
                  int priority = 50,
                  std::string condition = "",
                  std::function<void(EventContext&)> pOnRun = {})
        : EventBase(std::to_string(id), std::to_string(id), {}, triggers, condition, {}, "", "",
                    priority, "", announcement),
          log(pLog), id_num(id), on_run(std::move(pOnRun))
    {}

protected:
    void execute_(EventContext& ctx) override
    {
        log.push_back(id_num);
        if (on_run) on_run(ctx);
    }
};

struct ScopedEvalAtom
{
    Expression::EvalAtomFn previous;
    explicit ScopedEvalAtom(Expression::EvalAtomFn fn) : previous(Expression::set_eval_atom(std::move(fn))) {}
    ~ScopedEvalAtom() { Expression::set_eval_atom(previous); }
};

TEST_CASE("Announce - reactions run in sorted order before the announcer, before pending work", "[handler][announce]")
{
    clear_all();
    std::vector<int> log;

    auto r1 = std::make_shared<ScriptedEvent>(11, log, std::vector<std::string>{"OnCast"}, "", 60);
    auto r2 = std::make_shared<ScriptedEvent>(12, log, std::vector<std::string>{"OnCast"}, "", 40);
    EventStore::register_event(r2);
    EventStore::register_event(r1);

    auto announcer = std::make_shared<ScriptedEvent>(1, log, std::vector<std::string>{}, "OnCast");
    auto pending = std::make_shared<ScriptedEvent>(2, log, std::vector<std::string>{});

    EventQueue q;
    q.push_normal_event(QueuedEvent{announcer});
    q.push_normal_event(QueuedEvent{pending});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log == std::vector<int>{11, 12, 1, 2});
}

TEST_CASE("Announce - a reaction enabled by an earlier reaction still runs (judged at pop)", "[handler][announce]")
{
    clear_all();
    std::vector<int> log;
    static bool burning = false;
    burning = false;

    ScopedEvalAtom atoms([](const std::string& type, const std::string& value, const EventContext&) {
        return type == "STATUS" && value == "burning" && burning;
    });

    // Ignite sets the target alight; Flameburst only works on a burning target.
    auto ignite = std::make_shared<ScriptedEvent>(11, log, std::vector<std::string>{"OnCast"}, "", 60, "",
                                                  [](EventContext&) { burning = true; });
    auto flameburst = std::make_shared<ScriptedEvent>(12, log, std::vector<std::string>{"OnCast"}, "", 40,
                                                      R"({"STATUS","burning"})");
    EventStore::register_event(ignite);
    EventStore::register_event(flameburst);

    auto spell = std::make_shared<ScriptedEvent>(1, log, std::vector<std::string>{}, "OnCast");
    EventQueue q;
    q.push_normal_event(QueuedEvent{spell});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log == std::vector<int>{11, 12, 1});
}

TEST_CASE("Announce - without the enabling reaction, the conditional reaction does not run", "[handler][announce]")
{
    clear_all();
    std::vector<int> log;
    static bool burning = false;
    burning = false;

    ScopedEvalAtom atoms([](const std::string& type, const std::string& value, const EventContext&) {
        return type == "STATUS" && value == "burning" && burning;
    });

    auto flameburst = std::make_shared<ScriptedEvent>(12, log, std::vector<std::string>{"OnCast"}, "", 40,
                                                      R"({"STATUS","burning"})");
    EventStore::register_event(flameburst);

    auto spell = std::make_shared<ScriptedEvent>(1, log, std::vector<std::string>{}, "OnCast");
    EventQueue q;
    q.push_normal_event(QueuedEvent{spell});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log == std::vector<int>{1});
}

TEST_CASE("Announce - a reaction can cancel the announcer", "[handler][announce]")
{
    clear_all();
    std::vector<int> log;

    auto counterspell = std::make_shared<ScriptedEvent>(11, log, std::vector<std::string>{"OnCast"}, "", 50, "",
                                                        [](EventContext& ctx) { ctx.cancel_definition("1"); });
    EventStore::register_event(counterspell);

    auto spell = std::make_shared<ScriptedEvent>(1, log, std::vector<std::string>{}, "OnCast");
    auto pending = std::make_shared<ScriptedEvent>(2, log, std::vector<std::string>{});
    EventQueue q;
    q.push_normal_event(QueuedEvent{spell});
    q.push_normal_event(QueuedEvent{pending});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log == std::vector<int>{11, 2});
}

TEST_CASE("Announce - a reaction that announces nests its own burst", "[handler][announce]")
{
    clear_all();
    std::vector<int> log;

    // B reacts to A's announcement and announces itself; C reacts to B's; D reacts to A's after B.
    auto b = std::make_shared<ScriptedEvent>(2, log, std::vector<std::string>{"AnnA"}, "AnnB", 60);
    auto d = std::make_shared<ScriptedEvent>(4, log, std::vector<std::string>{"AnnA"}, "", 40);
    auto c = std::make_shared<ScriptedEvent>(3, log, std::vector<std::string>{"AnnB"});
    EventStore::register_event(b);
    EventStore::register_event(c);
    EventStore::register_event(d);

    auto a = std::make_shared<ScriptedEvent>(1, log, std::vector<std::string>{}, "AnnA");
    EventQueue q;
    q.push_normal_event(QueuedEvent{a});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log == std::vector<int>{3, 2, 4, 1});
}

TEST_CASE("Announce - an announcer with no listeners runs once", "[handler][announce]")
{
    clear_all();
    std::vector<int> log;

    auto a = std::make_shared<ScriptedEvent>(1, log, std::vector<std::string>{}, "Nobody");
    EventQueue q;
    q.push_normal_event(QueuedEvent{a});

    EventQueueHandler handler;
    handler.process(q, make_ctx());

    REQUIRE(log == std::vector<int>{1});
}

TEST_CASE("Announce - an event reacting to its own announcement is stopped", "[handler][announce]")
{
    clear_all();
    std::vector<int> log;

    // Listens to OnEcho and announces OnEcho: every announcement queues a fresh copy of itself.
    auto echo = std::make_shared<ScriptedEvent>(7, log, std::vector<std::string>{"OnEcho"}, "OnEcho");
    EventStore::register_event(echo);

    EventQueue q;
    q.push_normal_event(QueuedEvent{echo});

    auto ctx = make_ctx_with_width(5);
    EventQueueHandler handler;
    REQUIRE_NOTHROW(handler.process(q, ctx));

    REQUIRE(ctx->is_definition_banned("7"));
    REQUIRE(log.empty());   // banned here before any copy got past its announcement
}

// ===========================
// Destroyed events
// ===========================
class ActingEvent : public EventBase
{
public:
    std::vector<int>& log;
    int id_num;
    std::function<void()> action;

    ActingEvent(int id, std::vector<int>& pLog, std::function<void()> pAction,
                std::vector<std::string> triggers = {}, std::string follow_up = "")
        : EventBase(std::to_string(id), std::to_string(id), {}, triggers, "", {}, "", follow_up),
          log(pLog), id_num(id), action(std::move(pAction))
    {}

protected:
    void execute_(EventContext&) override
    {
        if (action) action();
        log.push_back(id_num);   // runs after the action may have destroyed this event's last owner
    }
};

TEST_CASE("Destroyed - an event destroyed while queued is skipped", "[handler][destroyed]")
{
    clear_all();
    std::vector<int> log;

    auto b = std::make_shared<TrackingEvent>(2, log);
    auto a = std::make_shared<ActingEvent>(1, log, [&b]() { b.reset(); });   // e.g. A kills B's owner

    EventQueue q;
    q.push_normal_event(QueuedEvent{a});
    q.push_normal_event(QueuedEvent{b});

    EventQueueHandler handler;
    REQUIRE_NOTHROW(handler.process(q, make_ctx()));

    REQUIRE(log == std::vector<int>{1});
}

TEST_CASE("Destroyed - a reaction whose event is gone before its turn does not run", "[handler][destroyed]")
{
    clear_all();
    std::vector<int> log;

    auto listener = std::make_shared<TrackingEvent>(2, log, "", std::vector<std::string>{"T"});
    EventStore::register_event(listener);

    auto ctx = make_ctx();
    EventQueue q;
    EventFiring::queue_trigger(q, ctx, "T");
    listener.reset();

    EventQueueHandler handler;
    handler.process(q, ctx);

    REQUIRE(log.empty());
}

TEST_CASE("Destroyed - an announcer destroyed by its own reaction is skipped", "[handler][destroyed]")
{
    clear_all();
    std::vector<int> log;

    std::shared_ptr<ScriptedEvent> announcer;
    auto killer = std::make_shared<ScriptedEvent>(11, log, std::vector<std::string>{"OnCast"}, "", 50, "",
                                                  [&announcer](EventContext&) { announcer.reset(); });
    EventStore::register_event(killer);

    announcer = std::make_shared<ScriptedEvent>(1, log, std::vector<std::string>{}, "OnCast");
    auto pending = std::make_shared<ScriptedEvent>(2, log, std::vector<std::string>{});

    EventQueue q;
    q.push_normal_event(QueuedEvent{announcer});
    q.push_normal_event(QueuedEvent{pending});

    EventQueueHandler handler;
    REQUIRE_NOTHROW(handler.process(q, make_ctx()));

    REQUIRE(log == std::vector<int>{11, 2});
}

TEST_CASE("Destroyed - an event that destroys itself finishes and fires its follow-up", "[handler][destroyed]")
{
    clear_all();
    std::vector<int> log;
    bool expired_while_running = true;

    auto follow_up = std::make_shared<TrackingEvent>(2, log, "", std::vector<std::string>{"OnDone"});
    EventStore::register_event(follow_up);

    std::shared_ptr<ActingEvent> self;
    std::weak_ptr<ActingEvent> watch;
    self = std::make_shared<ActingEvent>(1, log, [&]() {
        self.reset();
        expired_while_running = watch.expired();
    }, std::vector<std::string>{}, "OnDone");
    watch = self;

    EventQueue q;
    q.push_normal_event(QueuedEvent{self});

    EventQueueHandler handler;
    REQUIRE_NOTHROW(handler.process(q, make_ctx()));

    REQUIRE_FALSE(expired_while_running);   // the handler keeps it alive while it runs
    REQUIRE(watch.expired());
    REQUIRE(log == std::vector<int>{1, 2});
}

