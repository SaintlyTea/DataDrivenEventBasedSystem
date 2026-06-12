#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "Scripts/Handlers/EventQueueProcessor.h"
#include "Scripts/Models/EventOpQueue.h"
#include "Scripts/Models/EventContext/EventContext.h"
#include "Scripts/Models/Events/EventBase.h"

// Minimal test event that records execution order
class TrackingEvent : public EventBase
{
public:
    int id;
    std::vector<int>& execution_log;

    TrackingEvent(int id, std::vector<int>& log)
        : EventBase("", "", "", ""), id(id), execution_log(log)
    {}

protected:
    void execute_(const EventContext& ctx) override
    {
        execution_log.push_back(id);
    }
};

static EventContext make_ctx()
{
    return EventContext("test", "unit_01");
}

// ===========================
// Basic execution
// ===========================
TEST_CASE("Processor - single FIFO event executes", "[processor]")
{
    std::vector<int> log;
    TrackingEvent e(1, log);

    EventOpQueue q;
    q.push_normal_op(&e);

    EventQueueProcessor processor;
    processor.process(q, make_ctx());

    REQUIRE(log.size() == 1);
    REQUIRE(log[0] == 1);
}

TEST_CASE("Processor - multiple FIFO events execute in order", "[processor]")
{
    std::vector<int> log;
    TrackingEvent e1(1, log), e2(2, log), e3(3, log);

    EventOpQueue q;
    q.push_normal_op(&e1);
    q.push_normal_op(&e2);
    q.push_normal_op(&e3);

    EventQueueProcessor processor;
    processor.process(q, make_ctx());

    REQUIRE(log == std::vector<int>{1, 2, 3});
}

TEST_CASE("Processor - LIFO events execute before FIFO", "[processor]")
{
    std::vector<int> log;
    TrackingEvent e1(1, log), e2(2, log), e3(3, log), e4(4, log);

    EventOpQueue q;
    q.push_normal_op(&e1);
    q.push_normal_op(&e2);
    q.push_barrier_op(&e3);
    q.push_barrier_op(&e4);

    EventQueueProcessor processor;
    processor.process(q, make_ctx());

    REQUIRE(log == std::vector<int>{4, 3, 1, 2});
}

TEST_CASE("Processor - empty queue processes without error", "[processor]")
{
    EventOpQueue q;
    EventQueueProcessor processor;

    REQUIRE_NOTHROW(processor.process(q, make_ctx()));
}

// ===========================
// Mid-process barrier injection
// ===========================

// An event that pushes a new barrier op when it executes
class InjectingEvent : public EventBase
{
public:
    int id;
    std::vector<int>& execution_log;
    EventBase* to_inject;
    EventOpQueue& queue;

    InjectingEvent(int id, std::vector<int>& log, EventBase* inject, EventOpQueue& q)
        : EventBase("", "", "", ""), id(id), execution_log(log), to_inject(inject), queue(q)
    {}

protected:
    void execute_(const EventContext& ctx) override
    {
        execution_log.push_back(id);
        if (to_inject)
            queue.push_barrier_op(to_inject);
    }
};

TEST_CASE("Processor - mid-process barrier injection executes before remaining FIFO", "[processor]")
{
    std::vector<int> log;
    EventOpQueue q;

    TrackingEvent e2(2, log);
    TrackingEvent e3(3, log);
    InjectingEvent e1(1, log, &e3, q); // e1 injects e3 as barrier when it executes

    q.push_normal_op(&e1);
    q.push_normal_op(&e2);

    EventQueueProcessor processor;
    processor.process(q, make_ctx());

    // e1 runs, injects e3 as barrier, e3 runs before e2
    REQUIRE(log == std::vector<int>{1, 3, 2});
}