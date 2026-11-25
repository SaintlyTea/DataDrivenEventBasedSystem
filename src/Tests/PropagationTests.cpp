#include <catch2/catch_test_macros.hpp>
#include <memory>

#include "Scripts/Models/EventContext/EventContext.h"
#include "Scripts/Models/ContextPayload/Utility/CancellationPayload.h"

// ===========================
// Test payloads, one per propagation mode
// ===========================
class SharedTestPayload : public ContextPayload
{
public:
    int value {0};

    std::string_view type_id() const override { return "SharedTestPayload"; }
    PropagationMode propagation() const override { return PropagationMode::SharedReference; }

    bool merge_from(const ContextPayload& other) override
    {
        if (other.type_id() != type_id()) return false;
        value += static_cast<const SharedTestPayload&>(other).value;
        return true;
    }
};

class CopyTestPayload : public ContextPayload
{
public:
    int value {0};

    std::string_view type_id() const override { return "CopyTestPayload"; }
    PropagationMode propagation() const override { return PropagationMode::Copy; }

    std::unique_ptr<ContextPayload> clone() const override
    {
        auto copy = std::make_unique<CopyTestPayload>();
        copy->value = value;
        return copy;
    }

    bool merge_from(const ContextPayload& other) override
    {
        if (other.type_id() != type_id()) return false;
        value += static_cast<const CopyTestPayload&>(other).value;
        return true;
    }
};

class LocalTestPayload : public ContextPayload
{
public:
    std::string_view type_id() const override { return "LocalTestPayload"; }
    bool merge_from(const ContextPayload& other) override { return other.type_id() == type_id(); }
};

static CancellationPayload& cancellations_of(EventContext& ctx)
{
    auto* payload = dynamic_cast<CancellationPayload*>(ctx.get_payload(CancellationPayload::KEY));
    REQUIRE(payload != nullptr);
    return *payload;
}

// ===========================
// Root context
// ===========================
TEST_CASE("Propagation - root context starts with an empty cancellation payload", "[propagation]")
{
    EventContext root("test", "unit_01");

    REQUIRE(root.has_payload(CancellationPayload::KEY));
    REQUIRE_FALSE(cancellations_of(root).is_cancelled("fireball", "fireball#1"));
}

// ===========================
// Shared reference
// ===========================
TEST_CASE("Propagation - cancel issued in derived context is visible in parent", "[propagation]")
{
    EventContext root("test", "unit_01");
    auto child = root.create_derived("reaction", "unit_02");

    child->cancel_instance("fireball#1", true);

    REQUIRE(cancellations_of(root).is_cancelled("fireball", "fireball#1"));
}

TEST_CASE("Propagation - cancel issued three levels down is visible at the root", "[propagation]")
{
    EventContext root("test", "unit_01");
    auto d1 = root.create_derived("d1", "unit_01");
    auto d2 = d1->create_derived("d2", "unit_01");
    auto d3 = d2->create_derived("d3", "unit_01");

    d3->cancel_definition("fireball", true);

    REQUIRE(cancellations_of(root).is_cancelled("fireball", "fireball#7"));
    REQUIRE(cancellations_of(*d1).is_cancelled("fireball", "fireball#7"));
}

TEST_CASE("Propagation - cancel issued in parent after derive is visible in child", "[propagation]")
{
    EventContext root("test", "unit_01");
    auto child = root.create_derived("reaction", "unit_02");

    root.cancel_instance("fireball#1", true);

    REQUIRE(cancellations_of(*child).is_cancelled("fireball", "fireball#1"));
}

TEST_CASE("Propagation - one-time cancel consumed in child is gone in parent", "[propagation]")
{
    EventContext root("test", "unit_01");
    root.cancel_instance("fireball#1", false);
    auto child = root.create_derived("reaction", "unit_02");

    REQUIRE(child->is_event_cancelled("fireball", "fireball#1"));
    REQUIRE_FALSE(cancellations_of(root).is_cancelled("fireball", "fireball#1"));
}

TEST_CASE("Propagation - set_payload on a shared payload merges into the one object", "[propagation]")
{
    EventContext root("test", "unit_01");
    auto first = std::make_unique<SharedTestPayload>();
    first->value = 1;
    root.set_payload("shared", std::move(first));

    auto child = root.create_derived("reaction", "unit_02");
    auto more = std::make_unique<SharedTestPayload>();
    more->value = 5;
    child->set_payload("shared", std::move(more));

    REQUIRE(root.get_payload("shared") == child->get_payload("shared"));
    REQUIRE(static_cast<SharedTestPayload*>(root.get_payload("shared"))->value == 6);
}

// ===========================
// Copy
// ===========================
TEST_CASE("Propagation - copy payload is a snapshot; child changes do not reach parent", "[propagation]")
{
    EventContext root("test", "unit_01");
    auto data = std::make_unique<CopyTestPayload>();
    data->value = 3;
    root.set_payload("copy", std::move(data));

    auto child = root.create_derived("reaction", "unit_02");
    auto* child_copy = static_cast<CopyTestPayload*>(child->get_payload("copy"));
    REQUIRE(child_copy != nullptr);
    REQUIRE(child_copy->value == 3);

    child_copy->value = 99;

    REQUIRE(child->get_payload("copy") != root.get_payload("copy"));
    REQUIRE(static_cast<CopyTestPayload*>(root.get_payload("copy"))->value == 3);
}

// ===========================
// None
// ===========================
TEST_CASE("Propagation - non-propagating payload is left behind", "[propagation]")
{
    EventContext root("test", "unit_01");
    root.set_payload("local", std::make_unique<LocalTestPayload>());

    auto child = root.create_derived("reaction", "unit_02");

    REQUIRE(root.has_payload("local"));
    REQUIRE_FALSE(child->has_payload("local"));
}

// ===========================
// CancellationPayload rules
// ===========================
TEST_CASE("Cancellation - definition cancel matches any instance of that definition", "[cancellation]")
{
    CancellationPayload c;
    c.cancel_definition("fireball", true);

    REQUIRE(c.is_cancelled("fireball", "fireball#1"));
    REQUIRE(c.is_cancelled("fireball", "fireball#2"));
    REQUIRE_FALSE(c.is_cancelled("frostbolt", "frostbolt#1"));
}

TEST_CASE("Cancellation - instance cancel matches only that instance", "[cancellation]")
{
    CancellationPayload c;
    c.cancel_instance("fireball#1", true);

    REQUIRE(c.is_cancelled("fireball", "fireball#1"));
    REQUIRE_FALSE(c.is_cancelled("fireball", "fireball#2"));
}

TEST_CASE("Cancellation - each one-time cancel blocks exactly one occurrence, instance entry first", "[cancellation]")
{
    CancellationPayload c;
    c.cancel_instance("fireball#1", false);
    c.cancel_definition("fireball", false);

    REQUIRE(c.check_and_consume("fireball", "fireball#1"));
    REQUIRE_FALSE(c.is_cancelled("unrelated", "fireball#1"));   // probes the instance entry alone
    REQUIRE(c.is_cancelled("fireball", "fireball#2"));           // definition entry still there

    REQUIRE(c.check_and_consume("fireball", "fireball#1"));
    REQUIRE_FALSE(c.check_and_consume("fireball", "fireball#1"));
    REQUIRE_FALSE(c.is_cancelled("fireball", "fireball#2"));
}

TEST_CASE("Cancellation - a permanent definition cancel is never consumed and blocks only that definition", "[cancellation]")
{
    CancellationPayload c;
    c.cancel_definition("fireball", true);

    REQUIRE(c.check_and_consume("fireball", "fireball#1"));
    REQUIRE(c.check_and_consume("fireball", "fireball#1"));
    REQUIRE(c.check_and_consume("fireball", "fireball#2"));
    REQUIRE_FALSE(c.check_and_consume("frostbolt", "frostbolt#1"));
}

TEST_CASE("Cancellation - a permanent instance cancel is never consumed and blocks only that instance", "[cancellation]")
{
    CancellationPayload c;
    c.cancel_instance("fireball#1", true);

    REQUIRE(c.check_and_consume("fireball", "fireball#1"));
    REQUIRE(c.check_and_consume("fireball", "fireball#1"));
    REQUIRE_FALSE(c.check_and_consume("fireball", "fireball#2"));
}

TEST_CASE("Cancellation - an unrelated event is not blocked and uses up no entry", "[cancellation]")
{
    CancellationPayload c;
    c.cancel_instance("fireball#1", false);

    REQUIRE_FALSE(c.check_and_consume("fireball", "fireball#2"));
    REQUIRE_FALSE(c.check_and_consume("frostbolt", "frostbolt#1"));
    REQUIRE(c.check_and_consume("fireball", "fireball#1"));
}

TEST_CASE("Cancellation - a later one-time cancel does not downgrade a permanent one", "[cancellation]")
{
    CancellationPayload c;
    c.cancel_instance("fireball#1", true);
    c.cancel_instance("fireball#1", false);

    REQUIRE(c.check_and_consume("fireball", "fireball#1"));
    REQUIRE(c.check_and_consume("fireball", "fireball#1"));
    REQUIRE(c.is_cancelled("fireball", "fireball#1"));
}

TEST_CASE("Cancellation - merging keeps permanent over one-time in both directions", "[cancellation]")
{
    CancellationPayload upgraded;
    upgraded.cancel_definition("fireball", false);
    CancellationPayload permanent;
    permanent.cancel_definition("fireball", true);
    REQUIRE(upgraded.merge_from(permanent));

    CancellationPayload kept;
    kept.cancel_definition("fireball", true);
    CancellationPayload one_time;
    one_time.cancel_definition("fireball", false);
    REQUIRE(kept.merge_from(one_time));

    for (CancellationPayload* c : {&upgraded, &kept})
    {
        REQUIRE(c->check_and_consume("fireball", "fireball#1"));
        REQUIRE(c->check_and_consume("fireball", "fireball#2"));
        REQUIRE(c->is_cancelled("fireball", "fireball#3"));
    }
}
