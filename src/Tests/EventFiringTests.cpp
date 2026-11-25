#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <vector>

#include "Scripts/Handlers/EventFiring.h"
#include "Scripts/States/EventStore.h"

class FiringTestEvent : public EventBase
{
public:
    std::vector<int>& log;
    std::vector<int>& depths;
    int id_num;

    FiringTestEvent(int id, std::vector<int>& pLog, std::vector<int>& pDepths,
                    std::vector<std::string> triggers, int priority = 50)
        : EventBase(std::to_string(id), std::to_string(id), {}, triggers, "", {}, "", "", priority),
          log(pLog), depths(pDepths), id_num(id)
    {}

protected:
    void execute_(EventContext& ctx) override
    {
        log.push_back(id_num);
        depths.push_back(ctx.depth_tracker->current_depth);
    }
};

static std::shared_ptr<EventContext> make_ctx()
{
    return std::make_shared<EventContext>("test", "unit_01");
}

// ===========================
// get_sorted_events_for_trigger
// ===========================
TEST_CASE("EventFiring - no listeners gives an empty list", "[firing]")
{
    EventStore::clear();
    REQUIRE(EventFiring::get_sorted_events_for_trigger("OnAttack", *make_ctx()).empty());
}

TEST_CASE("EventFiring - fetches only the trigger's listeners, sorted", "[firing]")
{
    EventStore::clear();
    std::vector<int> log, depths;
    auto low   = std::make_shared<FiringTestEvent>(1, log, depths, std::vector<std::string>{"OnAttack"}, 10);
    auto high  = std::make_shared<FiringTestEvent>(2, log, depths, std::vector<std::string>{"OnAttack"}, 90);
    auto other = std::make_shared<FiringTestEvent>(3, log, depths, std::vector<std::string>{"OnDefend"});
    EventStore::register_event(low);
    EventStore::register_event(high);
    EventStore::register_event(other);

    auto results = EventFiring::get_sorted_events_for_trigger("OnAttack", *make_ctx());

    REQUIRE(results.size() == 2);
    REQUIRE(results[0] == high);
    REQUIRE(results[1] == low);
}

// ===========================
// queue_trigger
// ===========================
TEST_CASE("EventFiring - queue_trigger places events in LIFO with the existing context as triggering context", "[firing]")
{
    EventStore::clear();
    std::vector<int> log, depths;
    auto listener = std::make_shared<FiringTestEvent>(2, log, depths, std::vector<std::string>{"OnFollowUp"});
    EventStore::register_event(listener);

    auto ctx = make_ctx();
    EventQueue q;
    EventFiring::queue_trigger(q, ctx, "OnFollowUp");

    REQUIRE(q.lifo_size() == 1);
    REQUIRE(q.fifo_size() == 0);

    QueuedEvent queued = q.pop_next();
    REQUIRE(queued.event.lock() == listener);
    REQUIRE(queued.triggering_context.get() == ctx.get());
}

TEST_CASE("EventFiring - queue_trigger puts the first sorted event on top", "[firing]")
{
    EventStore::clear();
    std::vector<int> log, depths;
    auto low  = std::make_shared<FiringTestEvent>(1, log, depths, std::vector<std::string>{"T"}, 10);
    auto high = std::make_shared<FiringTestEvent>(2, log, depths, std::vector<std::string>{"T"}, 90);
    EventStore::register_event(low);
    EventStore::register_event(high);

    EventQueue q;
    EventFiring::queue_trigger(q, make_ctx(), "T");

    REQUIRE(q.pop_next().event.lock() == high);
    REQUIRE(q.pop_next().event.lock() == low);
}

// ===========================
// fire_trigger
// ===========================
TEST_CASE("EventFiring - fire_trigger runs every listener in sorted order, then unwinds", "[firing]")
{
    EventStore::clear();
    std::vector<int> log, depths;
    auto low  = std::make_shared<FiringTestEvent>(1, log, depths, std::vector<std::string>{"T"}, 10);
    auto high = std::make_shared<FiringTestEvent>(2, log, depths, std::vector<std::string>{"T"}, 90);
    EventStore::register_event(low);
    EventStore::register_event(high);

    auto ctx = make_ctx();
    EventFiring::fire_trigger(ctx, "T");

    REQUIRE(log == std::vector<int>{2, 1});
    REQUIRE(depths == std::vector<int>{0, 0});
    REQUIRE(ctx->depth_tracker->current_depth == -1);
}

TEST_CASE("EventFiring - fire_trigger with no listeners does nothing", "[firing]")
{
    EventStore::clear();
    auto ctx = make_ctx();
    REQUIRE_NOTHROW(EventFiring::fire_trigger(ctx, "Nothing"));
    REQUIRE(ctx->depth_tracker->current_depth == -1);
}
