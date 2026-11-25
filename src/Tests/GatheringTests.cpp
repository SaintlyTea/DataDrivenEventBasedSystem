#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <memory>
#include <vector>

#include "Scripts/Handlers/EventFiring.h"
#include "Scripts/Handlers/EventQueueHandler.h"
#include "Scripts/Models/ContextPayload/Combat/Damage/DamageCalcData.h"
#include "Scripts/Models/ContextPayload/Utility/CancellationPayload.h"
#include "Scripts/Models/EventContext/Combat/EventCombatContext.h"
#include "Scripts/Models/Events/CombatEvents/EventCombatBase.h"
#include "Scripts/States/EventStore.h"

// ===========================
// Test events
// ===========================
using Action = std::function<void(EventContext&)>;

class LambdaEvent : public EventBase
{
public:
    Action action;

    LambdaEvent(std::string id, std::vector<std::string> triggers, Action pAction, std::string follow_up = "")
        : EventBase(id, id, {}, triggers, "", {}, "", follow_up), action(std::move(pAction)) {}

protected:
    void execute_(EventContext& ctx) override { action(ctx); }
};

class LambdaCombatEvent : public EventCombatBase
{
public:
    Action action;

    LambdaCombatEvent(std::string id, std::vector<std::string> triggers, Action pAction)
        : EventCombatBase(id, id, {}, {}, triggers, ""), action(std::move(pAction)) {}

protected:
    void execute_combat_(EventCombatContext& ctx) override { action(ctx); }
};

// Copy-mode payload used to see which context a follow-up was derived from.
class CopyMarker : public ContextPayload
{
public:
    int value {0};

    std::string_view type_id() const override { return "CopyMarker"; }
    PropagationMode propagation() const override { return PropagationMode::Copy; }
    std::unique_ptr<ContextPayload> clone() const override
    {
        auto copy = std::make_unique<CopyMarker>();
        copy->value = value;
        return copy;
    }
    bool merge_from(const ContextPayload& other) override
    {
        if (other.type_id() != type_id()) return false;
        value = static_cast<const CopyMarker&>(other).value;
        return true;
    }
};

static DamageCalcData* damage_of(const EventContext& ctx)
{
    return dynamic_cast<DamageCalcData*>(ctx.get_payload("damage"));
}

static void add_flat(EventContext& ctx, float amount)
{
    auto add = std::make_shared<DamageCalcData>();
    add->flat = amount;
    ctx.set_payload("damage", add);
}

static std::shared_ptr<EventContext> caller_with_damage()
{
    auto caller = std::make_shared<EventContext>("DealDamage", "unit_01");
    caller->set_payload("damage", std::make_shared<DamageCalcData>());
    return caller;
}

// ===========================
// Derivation
// ===========================
TEST_CASE("Gather - a gather payload is shared only through a gathering derive", "[gather]")
{
    auto caller = caller_with_damage();

    auto modifier_ctx = caller->create_derived("mod", "unit_01", DerivationKind::Gather);
    auto reaction_ctx = caller->create_derived("reaction", "unit_01");

    REQUIRE(modifier_ctx->get_payload("damage") == caller->get_payload("damage"));
    REQUIRE_FALSE(reaction_ctx->has_payload("damage"));
}

// ===========================
// gather_trigger
// ===========================
TEST_CASE("Gather - modifiers add into the caller's payload from their own contexts", "[gather]")
{
    EventStore::clear();
    std::vector<const EventContext*> ran_on;

    auto plus2 = std::make_shared<LambdaEvent>("plus2", std::vector<std::string>{"GatherDamage"},
        [&](EventContext& ctx) { ran_on.push_back(&ctx); add_flat(ctx, 2); });
    auto plus3 = std::make_shared<LambdaEvent>("plus3", std::vector<std::string>{"GatherDamage"},
        [&](EventContext& ctx) { ran_on.push_back(&ctx); add_flat(ctx, 3); });
    EventStore::register_event(plus2);
    EventStore::register_event(plus3);

    auto caller = caller_with_damage();
    EventFiring::gather_trigger(caller, "GatherDamage");

    REQUIRE(damage_of(*caller)->flat == 5.0f);
    REQUIRE(ran_on.size() == 2);
    REQUIRE(ran_on[0] != caller.get());
    REQUIRE(ran_on[1] != caller.get());
    REQUIRE(ran_on[0] != ran_on[1]);
}

TEST_CASE("Gather - what a modifier triggers does not get the payload", "[gather]")
{
    EventStore::clear();
    bool follow_up_saw_damage = true;

    // The modifier's follow-up deals damage of its own: it must not see the outer calculation.
    auto modifier = std::make_shared<LambdaEvent>("mod", std::vector<std::string>{"GatherDamage"},
        [](EventContext& ctx) { add_flat(ctx, 2); }, "OnModifierUsed");
    auto own_damage = std::make_shared<LambdaEvent>("own_damage", std::vector<std::string>{"OnModifierUsed"},
        [&](EventContext& ctx) {
            follow_up_saw_damage = ctx.has_payload("damage");
            if (follow_up_saw_damage) add_flat(ctx, 100);
        });
    EventStore::register_event(modifier);
    EventStore::register_event(own_damage);

    auto caller = caller_with_damage();
    EventFiring::gather_trigger(caller, "GatherDamage");

    REQUIRE_FALSE(follow_up_saw_damage);
    REQUIRE(damage_of(*caller)->flat == 2.0f);
}

TEST_CASE("Gather - a modifier that gathers for itself uses its own payload, then writes back", "[gather]")
{
    EventStore::clear();
    const DamageCalcData* outer_payload = nullptr;
    const DamageCalcData* inner_touched = nullptr;

    // M gathers its own modifiers into a fresh payload, then adds twice their total to the outer one.
    auto nested = std::make_shared<LambdaEvent>("nested", std::vector<std::string>{"GatherDamage"},
        [](EventContext& ctx) {
            std::shared_ptr<EventContext> inner = ctx.create_derived("inner", ctx.trigger_source);
            REQUIRE_FALSE(inner->has_payload("damage"));
            inner->set_payload("damage", std::make_shared<DamageCalcData>());

            EventFiring::gather_trigger(inner, "GatherInner");

            add_flat(ctx, damage_of(*inner)->flat * 2);
        });
    auto plus1 = std::make_shared<LambdaEvent>("plus1", std::vector<std::string>{"GatherDamage"},
        [](EventContext& ctx) { add_flat(ctx, 1); });
    auto inner_plus10 = std::make_shared<LambdaEvent>("inner_plus10", std::vector<std::string>{"GatherInner"},
        [&](EventContext& ctx) { inner_touched = damage_of(ctx); add_flat(ctx, 10); });
    EventStore::register_event(nested);
    EventStore::register_event(plus1);
    EventStore::register_event(inner_plus10);

    auto caller = caller_with_damage();
    outer_payload = damage_of(*caller);
    EventFiring::gather_trigger(caller, "GatherDamage");

    REQUIRE(damage_of(*caller)->flat == 21.0f);
    REQUIRE(inner_touched != nullptr);
    REQUIRE(inner_touched != outer_payload);
}

// ===========================
// fire_trigger / queue_trigger build each event's own context
// ===========================
TEST_CASE("Gather - a non-gathering sub-run does not pass the payload, and contexts are type-correct", "[gather]")
{
    EventStore::clear();
    bool plain_saw_damage = true;
    bool combat_ran = false;

    auto plain = std::make_shared<LambdaEvent>("plain", std::vector<std::string>{"OnHit"},
        [&](EventContext& ctx) { plain_saw_damage = ctx.has_payload("damage"); });
    auto combat = std::make_shared<LambdaCombatEvent>("combat", std::vector<std::string>{"OnHit"},
        [&](EventContext&) { combat_ran = true; });
    EventStore::register_event(plain);
    EventStore::register_event(combat);

    EventFiring::fire_trigger(caller_with_damage(), "OnHit");

    REQUIRE_FALSE(plain_saw_damage);
    REQUIRE(combat_ran);   // only possible on an EventCombatContext
}

TEST_CASE("Gather - queue_trigger gives each entry its own context derived from the cause", "[gather]")
{
    EventStore::clear();
    auto combat = std::make_shared<LambdaCombatEvent>("combat", std::vector<std::string>{"T"}, [](EventContext&) {});
    EventStore::register_event(combat);

    auto cause = std::make_shared<EventContext>("cause", "unit_01");
    EventQueue q;
    EventFiring::queue_trigger(q, cause, "T");

    QueuedEvent queued = q.pop_next();
    REQUIRE(queued.own_context != nullptr);
    REQUIRE(queued.own_context != cause);
    REQUIRE(dynamic_cast<EventCombatContext*>(queued.own_context.get()) != nullptr);
    REQUIRE(queued.own_context->get_payload(CancellationPayload::KEY) == cause->get_payload(CancellationPayload::KEY));
    REQUIRE(queued.triggering_context == cause);
}

// ===========================
// Handler
// ===========================
TEST_CASE("Gather - a follow-up derives from the executing event's own context", "[gather]")
{
    EventStore::clear();
    int follow_up_saw = -1;

    auto follow_up = std::make_shared<LambdaEvent>("next", std::vector<std::string>{"Next"},
        [&](EventContext& ctx) {
            auto* marker = dynamic_cast<CopyMarker*>(ctx.get_payload("marker"));
            follow_up_saw = marker ? marker->value : 0;
        });
    EventStore::register_event(follow_up);

    auto first = std::make_shared<LambdaEvent>("first", std::vector<std::string>{}, [](EventContext& ctx) {
        auto marker = std::make_shared<CopyMarker>();
        marker->value = 7;
        ctx.set_payload("marker", marker);
    }, "Next");

    EventQueue q;
    q.push_normal_event(QueuedEvent{first});
    EventQueueHandler handler;
    handler.process(q, std::make_shared<EventContext>("root", "unit_01"));

    REQUIRE(follow_up_saw == 7);
}

TEST_CASE("Gather - an event seeded without a context does not run on the handler's context", "[gather]")
{
    EventStore::clear();
    const EventContext* ran_on = nullptr;
    auto e = std::make_shared<LambdaEvent>("e", std::vector<std::string>{}, [&](EventContext& ctx) { ran_on = &ctx; });

    auto handler_ctx = std::make_shared<EventContext>("root", "unit_01");
    EventQueue q;
    q.push_normal_event(QueuedEvent{e});
    EventQueueHandler handler;
    handler.process(q, handler_ctx);

    REQUIRE(ran_on != nullptr);
    REQUIRE(ran_on != handler_ctx.get());
}
