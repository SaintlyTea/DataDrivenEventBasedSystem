#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <vector>

#include "Scripts/Handlers/EventSorter.h"
#include "Scripts/Models/Entities/EventOwners/MockOwner.h"
#include "Scripts/Models/Events/CombatEvents/EventCombatBase.h"
#include "Scripts/Models/EventContext/Combat/EventCombatContext.h"

// Minimal combat event for testing
class TriggerTestEvent : public EventCombatBase
{
public:
    TriggerTestEvent(
        std::string pId,
        std::weak_ptr<IEventOwner> pSource,
        std::weak_ptr<IEventOwner> pOwner,
        std::vector<std::string> pTriggers,
        int pPriority = 50,
        std::string pInstanceId = "")
        : EventCombatBase(pId, pId, pSource, pOwner,
            pTriggers, "", TargetingStrategy::EXPLICIT, {}, "", "", pPriority,
            false, -1, 0, true, false, -1, 0, 0, pInstanceId)
    {}
};

static std::shared_ptr<MockOwner> make_owner(
    std::string def_id, 
    std::string inst_id,
    std::vector<std::string> event_ids = {})
{
    auto owner = std::make_shared<MockOwner>(def_id, inst_id, std::vector<std::string>{"1"});
    owner->registered_event_ids = event_ids;
    return owner;
}

static EventCombatContext make_ctx(
    std::string source_id = "source_01",
    std::vector<std::string> targets = {})
{
    return EventCombatContext("test", source_id, targets);
}

// ===========================
// Basics
// ===========================
TEST_CASE("EventSorter - empty list stays empty", "[sorter]")
{
    REQUIRE(EventSorter::sort({}, make_ctx()).empty());
}

// ===========================
// Priority (level 1)
// ===========================
TEST_CASE("EventSorter - higher priority first", "[sorter]")
{
    auto owner = make_owner("def_01", "inst_01");
    auto e1 = std::make_shared<TriggerTestEvent>("e1", owner, owner, std::vector<std::string>{"OnAttack"}, 10);
    auto e2 = std::make_shared<TriggerTestEvent>("e2", owner, owner, std::vector<std::string>{"OnAttack"}, 90);

    auto results = EventSorter::sort({e1, e2}, make_ctx());
    REQUIRE(results[0]->id == "e2");
    REQUIRE(results[1]->id == "e1");
}

// ===========================
// Source match (level 2)
// ===========================
TEST_CASE("EventSorter - owner matching source wins tie", "[sorter]")
{
    auto source_owner = make_owner("def_01", "source_01");
    auto other_owner  = make_owner("def_02", "other_01");

    auto e1 = std::make_shared<TriggerTestEvent>("e1", other_owner,  other_owner,  std::vector<std::string>{"OnAttack"});
    auto e2 = std::make_shared<TriggerTestEvent>("e2", source_owner, source_owner, std::vector<std::string>{"OnAttack"});

    auto results = EventSorter::sort({e1, e2}, make_ctx("source_01"));
    REQUIRE(results[0]->id == "e2");
    REQUIRE(results[1]->id == "e1");
}

// ===========================
// Target match (level 3)
// ===========================
TEST_CASE("EventSorter - owner matching target wins tie over neither", "[sorter]")
{
    auto target_owner = make_owner("def_01", "target_01");
    auto other_owner  = make_owner("def_02", "other_01");

    auto e1 = std::make_shared<TriggerTestEvent>("e1", other_owner,  other_owner,  std::vector<std::string>{"OnAttack"});
    auto e2 = std::make_shared<TriggerTestEvent>("e2", target_owner, target_owner, std::vector<std::string>{"OnAttack"});

    auto results = EventSorter::sort({e1, e2}, make_ctx("source_99", {"target_01"}));
    REQUIRE(results[0]->id == "e2");
    REQUIRE(results[1]->id == "e1");
}

TEST_CASE("EventSorter - no targets, target check skipped", "[sorter]")
{
    auto target_owner = make_owner("def_01", "target_01");
    auto other_owner  = make_owner("def_01", "other_01");

    auto a = std::make_shared<TriggerTestEvent>("a_event", other_owner,  other_owner,  std::vector<std::string>{"OnAttack"});
    auto b = std::make_shared<TriggerTestEvent>("b_event", target_owner, target_owner, std::vector<std::string>{"OnAttack"});

    REQUIRE(EventSorter::sort({b, a}, make_ctx("source_99"))[0]->id == "a_event");
    REQUIRE(EventSorter::sort({a, b}, make_ctx("source_99", {"target_01"}))[0]->id == "b_event");
}

// ===========================
// Event definition id (level 4)
// ===========================
TEST_CASE("EventSorter - lower event definition id first on tie", "[sorter]")
{
    auto owner = make_owner("def_01", "inst_01");
    auto e1 = std::make_shared<TriggerTestEvent>("b_event", owner, owner, std::vector<std::string>{"OnAttack"});
    auto e2 = std::make_shared<TriggerTestEvent>("a_event", owner, owner, std::vector<std::string>{"OnAttack"});

    auto results = EventSorter::sort({e1, e2}, make_ctx());
    REQUIRE(results[0]->id == "a_event");
    REQUIRE(results[1]->id == "b_event");
}

// ===========================
// compare_to (level 5)
// ===========================
class ComparableEvent : public EventCombatBase
{
public:
    int compare_value;

    ComparableEvent(
        std::string pId, 
        std::weak_ptr<IEventOwner> pOwner, 
        int compareVal)
        : EventCombatBase(pId, pId, pOwner, pOwner, {"OnAttack"}, "")
        , compare_value(compareVal)
    {}

    int compare_to(const EventBase& other) const override
    {
        const auto* o = dynamic_cast<const ComparableEvent*>(&other);
        if (!o) return 0;
        return compare_value - o->compare_value;
    }
};

TEST_CASE("EventSorter - compare_to respected on tie", "[sorter]")
{
    auto owner = make_owner("def_01", "inst_01");
    auto e1 = std::make_shared<ComparableEvent>("same_id", owner, 100);
    auto e2 = std::make_shared<ComparableEvent>("same_id", owner, 10);

    auto results = EventSorter::sort({e1, e2}, make_ctx());
    REQUIRE(static_cast<ComparableEvent*>(results[0].get())->compare_value == 10);
    REQUIRE(static_cast<ComparableEvent*>(results[1].get())->compare_value == 100);
}

// ===========================
// Owner definition id (level 6)
// ===========================
TEST_CASE("EventSorter - lower owner definition id first on tie", "[sorter]")
{
    auto owner_a = make_owner("a_def", "inst_01");
    auto owner_b = make_owner("b_def", "inst_02");

    auto e1 = std::make_shared<TriggerTestEvent>("same_id", owner_b, owner_b, std::vector<std::string>{"OnAttack"});
    auto e2 = std::make_shared<TriggerTestEvent>("same_id", owner_a, owner_a, std::vector<std::string>{"OnAttack"});

    auto results = EventSorter::sort({e1, e2}, make_ctx());
    REQUIRE(results[0]->source.lock()->get_definition_id() == "a_def");
    REQUIRE(results[1]->source.lock()->get_definition_id() == "b_def");
}

// ===========================
// Registration order (level 7)
// ===========================
TEST_CASE("EventSorter - earlier registration in the owner's list first on tie", "[sorter]")
{
    auto owner = make_owner("def_01", "inst_01", {"x#2", "x#1"});

    auto first_listed  = std::make_shared<TriggerTestEvent>("x", owner, owner, std::vector<std::string>{"OnAttack"}, 50, "x#2");
    auto second_listed = std::make_shared<TriggerTestEvent>("x", owner, owner, std::vector<std::string>{"OnAttack"}, 50, "x#1");

    auto results = EventSorter::sort({second_listed, first_listed}, make_ctx());
    REQUIRE(results[0]->instance_id == "x#2");
    REQUIRE(results[1]->instance_id == "x#1");
}

// ===========================
// Instance id (level 8)
// ===========================
TEST_CASE("EventSorter - lower instance id first as last resort", "[sorter]")
{
    auto owner_a = make_owner("def_01", "aaa_inst");
    auto owner_b = make_owner("def_01", "bbb_inst");

    auto e1 = std::make_shared<TriggerTestEvent>("same_id", owner_b, owner_b, std::vector<std::string>{"OnAttack"});
    auto e2 = std::make_shared<TriggerTestEvent>("same_id", owner_a, owner_a, std::vector<std::string>{"OnAttack"});

    auto results = EventSorter::sort({e1, e2}, make_ctx());
    REQUIRE(results[0]->source.lock()->get_instance_id() == "aaa_inst");
    REQUIRE(results[1]->source.lock()->get_instance_id() == "bbb_inst");
}

// ===========================
// Non-combat events: owner falls back to source (ADR-004)
// ===========================
class PlainEvent : public EventBase
{
public:
    PlainEvent(std::string pId, std::weak_ptr<IEventOwner> pSource)
        : EventBase(pId, pId, pSource, {"OnAttack"}, "") {}
};

TEST_CASE("EventSorter - non-combat event whose source caused the trigger wins level 2", "[sorter]")
{
    auto causer = make_owner("def_02", "source_01");
    auto other  = make_owner("def_01", "other_01");

    // Level 4 alone would put "a_event" first.
    auto a = std::make_shared<PlainEvent>("a_event", other);
    auto b = std::make_shared<PlainEvent>("b_event", causer);

    auto results = EventSorter::sort({a, b}, make_ctx("source_01"));
    REQUIRE(results[0]->id == "b_event");
}

TEST_CASE("EventSorter - non-combat event whose source is a target wins level 3", "[sorter]")
{
    auto target = make_owner("def_02", "target_01");
    auto other  = make_owner("def_01", "other_01");

    auto a = std::make_shared<PlainEvent>("a_event", other);
    auto b = std::make_shared<PlainEvent>("b_event", target);

    auto results = EventSorter::sort({a, b}, make_ctx("source_99", {"target_01"}));
    REQUIRE(results[0]->id == "b_event");
}
