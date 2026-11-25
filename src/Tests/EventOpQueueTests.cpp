#include <catch2/catch_test_macros.hpp>
#include "Scripts/Models/EventOpQueue.h"
#include "Scripts/Models/EventContext/EventContext.h"

// Minimal stub just for queue ordering tests
class TestEvent : public EventBase
{
public:
    int id;
    TestEvent(int id) 
        : EventBase("", "", "", "", ""), id(id) 
    {}
};

// Helper to make test events readable
static TestEvent* make_event(int id)
{
    static std::vector<std::unique_ptr<TestEvent>> events;
    events.push_back(std::make_unique<TestEvent>(id));
    return events.back().get();
}

// ===========================
// FIFO ordering
// ===========================
TEST_CASE("FIFO - items come out in order they went in", "[queue]")
{
    EventOpQueue q;
    q.push_normal_op(make_event(1));
    q.push_normal_op(make_event(2));
    q.push_normal_op(make_event(3));

    REQUIRE(static_cast<TestEvent*>(q.pop_next())->id == 1);
    REQUIRE(static_cast<TestEvent*>(q.pop_next())->id == 2);
    REQUIRE(static_cast<TestEvent*>(q.pop_next())->id == 3);
}

// ===========================
// LIFO ordering
// ===========================
TEST_CASE("LIFO - items come out in reverse order", "[queue]")
{
    EventOpQueue q;
    q.push_barrier_op(make_event(1));
    q.push_barrier_op(make_event(2));
    q.push_barrier_op(make_event(3));

    REQUIRE(static_cast<TestEvent*>(q.pop_next())->id == 3);
    REQUIRE(static_cast<TestEvent*>(q.pop_next())->id == 2);
    REQUIRE(static_cast<TestEvent*>(q.pop_next())->id == 1);
}

// ===========================
// LIFO drains before FIFO
// ===========================
TEST_CASE("LIFO drains before FIFO", "[queue]")
{
    EventOpQueue q;
    q.push_normal_op(make_event(1));
    q.push_normal_op(make_event(2));
    q.push_barrier_op(make_event(3));
    q.push_barrier_op(make_event(4));

    REQUIRE(static_cast<TestEvent*>(q.pop_next())->id == 4);
    REQUIRE(static_cast<TestEvent*>(q.pop_next())->id == 3);
    REQUIRE(static_cast<TestEvent*>(q.pop_next())->id == 1);
    REQUIRE(static_cast<TestEvent*>(q.pop_next())->id == 2);
}

// ===========================
// Batch operations
// ===========================
TEST_CASE("enqueue_normal_ops merges FIFO in order", "[queue]")
{
    EventOpQueue main_q;
    main_q.push_normal_op(make_event(1));

    EventOpQueue batch;
    batch.push_normal_op(make_event(2));
    batch.push_normal_op(make_event(3));

    main_q.enqueue_normal_ops(batch);

    REQUIRE(static_cast<TestEvent*>(main_q.pop_next())->id == 1);
    REQUIRE(static_cast<TestEvent*>(main_q.pop_next())->id == 2);
    REQUIRE(static_cast<TestEvent*>(main_q.pop_next())->id == 3);
}

TEST_CASE("enqueue_barrier_ops reverses batch into LIFO", "[queue]")
{
    EventOpQueue main_q;

    EventOpQueue batch;
    batch.push_normal_op(make_event(1));
    batch.push_normal_op(make_event(2));
    batch.push_normal_op(make_event(3));

    main_q.enqueue_barrier_ops(batch);

    REQUIRE(static_cast<TestEvent*>(main_q.pop_next())->id == 1);
    REQUIRE(static_cast<TestEvent*>(main_q.pop_next())->id == 2);
    REQUIRE(static_cast<TestEvent*>(main_q.pop_next())->id == 3);
}

// ===========================
// Edge cases
// ===========================
TEST_CASE("pop_next on empty queue throws", "[queue]")
{
    EventOpQueue q;
    REQUIRE_THROWS_AS(q.pop_next(), std::runtime_error);
}

TEST_CASE("is_empty correct before and after ops", "[queue]")
{
    EventOpQueue q;
    REQUIRE(q.is_empty() == true);

    q.push_normal_op(make_event(1));
    REQUIRE(q.is_empty() == false);

    q.pop_next();
    REQUIRE(q.is_empty() == true);
}

TEST_CASE("Mid-process push - new LIFO op executes before remaining FIFO", "[queue]")
{
    EventOpQueue q;
    q.push_normal_op(make_event(1));
    q.push_normal_op(make_event(2));

    REQUIRE(static_cast<TestEvent*>(q.pop_next())->id == 1);

    q.push_barrier_op(make_event(3));

    REQUIRE(static_cast<TestEvent*>(q.pop_next())->id == 3);
    REQUIRE(static_cast<TestEvent*>(q.pop_next())->id == 2);
}