#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <vector>

#include "Scripts/States/EventStore.h"
#include "Scripts/Models/Events/EventBase.h"

// Minimal test event
class StoreTestEvent : public EventBase
{
public:
    StoreTestEvent(
        std::string pId,
        std::string pName,
        std::vector<std::string> pTriggers)
        : EventBase(pId, pName, std::weak_ptr<IEventOwner>{}, pTriggers, "", {})
    {}
};

static void clear_store()
{
    EventStore::clear();
}

// ===========================
// Registration - owned
// ===========================
TEST_CASE("EventStore - register owned event, appears in query", "[eventstore]")
{
    clear_store();
    auto event = std::make_shared<StoreTestEvent>("e1", "TestEvent", std::vector<std::string>{"OnAttack"});
    EventStore::register_event(event);

    auto results = EventStore::get_events_for_trigger("OnAttack");
    REQUIRE(results.size() == 1);
    REQUIRE(results[0]->id == "e1");
}

TEST_CASE("EventStore - register event with multiple triggers, appears under all", "[eventstore]")
{
    clear_store();
    auto event = std::make_shared<StoreTestEvent>("e1", "TestEvent", std::vector<std::string>{"OnAttack", "OnTurnStart"});
    EventStore::register_event(event);

    auto r1 = EventStore::get_events_for_trigger("OnAttack");
    auto r2 = EventStore::get_events_for_trigger("OnTurnStart");

    REQUIRE(r1.size() == 1);
    REQUIRE(r2.size() == 1);
    REQUIRE(r1[0]->id == "e1");
    REQUIRE(r2[0]->id == "e1");
}

TEST_CASE("EventStore - register two events under same trigger, both appear", "[eventstore]")
{
    clear_store();
    auto e1 = std::make_shared<StoreTestEvent>("e1", "Event1", std::vector<std::string>{"OnAttack"});
    auto e2 = std::make_shared<StoreTestEvent>("e2", "Event2", std::vector<std::string>{"OnAttack"});

    EventStore::register_event(e1);
    EventStore::register_event(e2);

    auto results = EventStore::get_events_for_trigger("OnAttack");
    REQUIRE(results.size() == 2);
}

TEST_CASE("EventStore - query trigger with no registered events returns empty", "[eventstore]")
{
    clear_store();
    auto results = EventStore::get_events_for_trigger("OnAttack");
    REQUIRE(results.empty());
}

// ===========================
// Registration - permanent
// ===========================
TEST_CASE("EventStore - register permanent event, appears in query", "[eventstore]")
{
    clear_store();
    auto event = std::make_shared<StoreTestEvent>("e1", "PermanentEvent", std::vector<std::string>{"OnDeath"});
    EventStore::register_permanent_event(event);

    auto results = EventStore::get_events_for_trigger("OnDeath");
    REQUIRE(results.size() == 1);
    REQUIRE(results[0]->id == "e1");
}

// ===========================
// Unregistration - owned
// ===========================
TEST_CASE("EventStore - unregister owned event, no longer appears", "[eventstore]")
{
    clear_store();
    auto event = std::make_shared<StoreTestEvent>("e1", "TestEvent", std::vector<std::string>{"OnAttack"});
    EventStore::register_event(event);
    EventStore::unregister_event(event);

    auto results = EventStore::get_events_for_trigger("OnAttack");
    REQUIRE(results.empty());
}

TEST_CASE("EventStore - unregister one of two events, only that one is gone", "[eventstore]")
{
    clear_store();
    auto e1 = std::make_shared<StoreTestEvent>("e1", "Event1", std::vector<std::string>{"OnAttack"});
    auto e2 = std::make_shared<StoreTestEvent>("e2", "Event2", std::vector<std::string>{"OnAttack"});

    EventStore::register_event(e1);
    EventStore::register_event(e2);
    EventStore::unregister_event(e1);

    auto results = EventStore::get_events_for_trigger("OnAttack");
    REQUIRE(results.size() == 1);
    REQUIRE(results[0]->id == "e2");
}

TEST_CASE("EventStore - unregister event with multiple triggers, gone from all", "[eventstore]")
{
    clear_store();
    auto event = std::make_shared<StoreTestEvent>("e1", "TestEvent", std::vector<std::string>{"OnAttack", "OnTurnStart"});
    EventStore::register_event(event);
    EventStore::unregister_event(event);

    REQUIRE(EventStore::get_events_for_trigger("OnAttack").empty());
    REQUIRE(EventStore::get_events_for_trigger("OnTurnStart").empty());
}

TEST_CASE("EventStore - unregister event never registered, no crash", "[eventstore]")
{
    clear_store();
    auto event = std::make_shared<StoreTestEvent>("e1", "TestEvent", std::vector<std::string>{"OnAttack"});
    REQUIRE_NOTHROW(EventStore::unregister_event(event));
}

// ===========================
// Unregistration - permanent
// ===========================
TEST_CASE("EventStore - unregister permanent event, no longer appears", "[eventstore]")
{
    clear_store();
    auto event = std::make_shared<StoreTestEvent>("e1", "PermanentEvent", std::vector<std::string>{"OnDeath"});
    EventStore::register_permanent_event(event);
    EventStore::unregister_permanent_event(event);

    auto results = EventStore::get_events_for_trigger("OnDeath");
    REQUIRE(results.empty());
}

// ===========================
// Expiry
// ===========================
TEST_CASE("EventStore - expired owned event not returned in query", "[eventstore]")
{
    clear_store();
    {
        auto event = std::make_shared<StoreTestEvent>("e1", "TestEvent", std::vector<std::string>{"OnAttack"});
        EventStore::register_event(event);
    } // event goes out of scope here, shared_ptr destroyed, weak_ptr expires

    auto results = EventStore::get_events_for_trigger("OnAttack");
    REQUIRE(results.empty());
}