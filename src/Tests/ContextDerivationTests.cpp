#include <catch2/catch_test_macros.hpp>
#include <memory>

#include "Scripts/Helpers/Context/ContextBuilder.h"
#include "Scripts/Models/EventContext/EventContext.h"
#include "Scripts/Models/EventContext/Combat/EventCombatContext.h"
#include "Scripts/Models/ContextPayload/Utility/CancellationPayload.h"
#include "Scripts/Models/Events/CombatEvents/EventCombatBase.h"

// ===========================
// Test context hierarchy
// ===========================
class TestContextA : public EventContext
{
public:
    int attr_a1 {0};

    TestContextA(std::string type, std::string source) : EventContext(std::move(type), std::move(source)) {}

    void derive_from(const EventContext& parent, DerivationKind kind) override
    {
        if (const auto* a = dynamic_cast<const TestContextA*>(&parent))
            attr_a1 = a->attr_a1;
        EventContext::derive_from(parent, kind);
    }
};

class TestContextB : public TestContextA
{
public:
    int attr_b1 {0};

    TestContextB(std::string type, std::string source) : TestContextA(std::move(type), std::move(source)) {}

    void derive_from(const EventContext& parent, DerivationKind kind) override
    {
        if (const auto* b = dynamic_cast<const TestContextB*>(&parent))
            attr_b1 = b->attr_b1;
        TestContextA::derive_from(parent, kind);
    }
};

static bool shares_universal_state(const EventContext& child, const EventContext& parent)
{
    return child.depth_tracker == parent.depth_tracker
        && child.get_payload(CancellationPayload::KEY) == parent.get_payload(CancellationPayload::KEY);
}

// ===========================
// Chain derivation
// ===========================
TEST_CASE("Derivation - same type copies every level's attributes", "[derivation]")
{
    TestContextB parent("p", "unit_01");
    parent.attr_a1 = 1;
    parent.attr_b1 = 2;

    TestContextB child("c", "unit_01");
    child.derive_from(parent, DerivationKind::Normal);

    REQUIRE(child.attr_a1 == 1);
    REQUIRE(child.attr_b1 == 2);
    REQUIRE(shares_universal_state(child, parent));
}

TEST_CASE("Derivation - from an ancestor type copies only the shared levels", "[derivation]")
{
    TestContextA parent("p", "unit_01");
    parent.attr_a1 = 1;

    TestContextB child("c", "unit_01");
    child.derive_from(parent, DerivationKind::Normal);

    REQUIRE(child.attr_a1 == 1);
    REQUIRE(child.attr_b1 == 0);
    REQUIRE(shares_universal_state(child, parent));
}

TEST_CASE("Derivation - from a descendant type copies the levels it has", "[derivation]")
{
    TestContextB parent("p", "unit_01");
    parent.attr_a1 = 1;
    parent.attr_b1 = 2;

    TestContextA child("c", "unit_01");
    child.derive_from(parent, DerivationKind::Normal);

    REQUIRE(child.attr_a1 == 1);
    REQUIRE(shares_universal_state(child, parent));
}

TEST_CASE("Derivation - from the base context carries only universal state", "[derivation]")
{
    EventContext parent("p", "unit_01");

    TestContextB child("c", "unit_01");
    child.derive_from(parent, DerivationKind::Normal);

    REQUIRE(child.attr_a1 == 0);
    REQUIRE(child.attr_b1 == 0);
    REQUIRE(shares_universal_state(child, parent));
}

TEST_CASE("Derivation - from an unrelated family carries only universal state", "[derivation]")
{
    EventCombatContext parent("p", "unit_01", {"t1"});

    TestContextB child("c", "unit_01");
    child.derive_from(parent, DerivationKind::Normal);

    REQUIRE(child.attr_a1 == 0);
    REQUIRE(child.attr_b1 == 0);
    REQUIRE(shares_universal_state(child, parent));
}

TEST_CASE("Derivation - a cancel issued in the derived context reaches the parent", "[derivation]")
{
    TestContextB parent("p", "unit_01");
    TestContextB child("c", "unit_01");
    child.derive_from(parent, DerivationKind::Normal);

    child.cancel_definition("fireball", true);

    REQUIRE(parent.is_event_cancelled("fireball", "fireball#1"));
}

// ===========================
// ContextBuilder: the event's class picks the context class
// ===========================
class PlainTestEvent : public EventBase
{
public:
    PlainTestEvent() : EventBase("plain", "plain", {}, {}, "", {}, "plain_type") {}
};

class CombatTestEvent : public EventCombatBase
{
public:
    CombatTestEvent() : EventCombatBase("combat", "combat", {}, {}, {}, "", TargetingStrategy::EXPLICIT, {}, "combat_type") {}
};

TEST_CASE("ContextBuilder - combat event gets a combat context", "[derivation][builder]")
{
    EventContext parent("p", "unit_01");
    CombatTestEvent event;

    auto ctx = ContextBuilder::build_for(event, parent);

    REQUIRE(dynamic_cast<EventCombatContext*>(ctx.get()) != nullptr);
    REQUIRE(ctx->event_type == "combat_type");
    REQUIRE(ctx->trigger_source == "unit_01");
    REQUIRE(shares_universal_state(*ctx, parent));
}

TEST_CASE("ContextBuilder - plain event gets a plain context, even from a combat parent", "[derivation][builder]")
{
    EventCombatContext parent("p", "unit_01", {"t1"});
    PlainTestEvent event;

    auto ctx = ContextBuilder::build_for(event, parent);

    REQUIRE(ctx != nullptr);
    REQUIRE(dynamic_cast<EventCombatContext*>(ctx.get()) == nullptr);
    REQUIRE(ctx->event_type == "plain_type");
    REQUIRE(shares_universal_state(*ctx, parent));
}
