#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Scripts/Handlers/EventFiring.h"
#include "Scripts/Handlers/EventQueueHandler.h"
#include "Scripts/Models/ContextPayload/Combat/Damage/DamageCalcData.h"
#include "Scripts/Models/ContextPayload/Utility/CancellationPayload.h"
#include "Scripts/Models/Expression.h"
#include "Scripts/States/EventStore.h"

namespace
{

// ===========================
// Test world and events
// ===========================
struct World
{
    std::map<std::string, int> hp {{"mage", 50}, {"goblin", 50}};
    bool focused {false};
    std::vector<std::string> log {};
};

class ActionEvent : public EventBase
{
public:
    World& world;
    std::function<void(EventContext&)> action;

    ActionEvent(World& pWorld, std::string id, std::vector<std::string> triggers,
                std::function<void(EventContext&)> pAction = {},
                std::string condition = "", std::string follow_up = "", int priority = 50)
        : EventBase(id, id, {}, triggers, condition, {}, "", follow_up, priority),
          world(pWorld), action(std::move(pAction)) {}

protected:
    void execute_(EventContext& ctx) override
    {
        world.log.push_back(id);
        if (action) action(ctx);
    }
};

// The target is fixed: targeting is a later branch.
class DealDamage : public EventBase
{
public:
    World& world;
    std::string target;
    float base;

    DealDamage(World& pWorld, std::string id, std::vector<std::string> triggers, std::string pTarget, float pBase,
               std::string announcement = "", std::string follow_up = "")
        : EventBase(id, id, {}, triggers, "", {}, "", follow_up, 50, "", announcement),
          world(pWorld), target(std::move(pTarget)), base(pBase) {}

protected:
    void execute_(EventContext& ctx) override
    {
        std::shared_ptr<EventContext> calc = ctx.create_derived(id, ctx.trigger_source);
        auto damage = std::make_shared<DamageCalcData>();
        damage->base = base;
        calc->set_payload("damage", damage);

        EventFiring::gather_trigger(calc, "OnGatherDamage");

        const int total = static_cast<int>(damage->base + damage->flat);
        world.hp[target] -= total;
        world.log.push_back(id + " hits " + target + " for " + std::to_string(total));
    }
};

void add_flat(EventContext& ctx, float amount)
{
    auto add = std::make_shared<DamageCalcData>();
    add->flat = amount;
    ctx.set_payload("damage", add);
}

struct ScopedEvalAtom
{
    Expression::EvalAtomFn previous;
    explicit ScopedEvalAtom(Expression::EvalAtomFn fn) : previous(Expression::set_eval_atom(std::move(fn))) {}
    ~ScopedEvalAtom() { Expression::set_eval_atom(previous); }
};

}

// ===========================
// One full run
// ===========================
TEST_CASE("EndToEnd - two spells with announcements, modifiers, a nested gather and a counterspell", "[e2e]")
{
    EventStore::clear();
    World world;
    ScopedEvalAtom atoms([&world](const std::string& type, const std::string& value, const EventContext&) {
        return type == "STATUS" && value == "focused" && world.focused;
    });

    auto arcane_focus = std::make_shared<ActionEvent>(world, "arcane_focus", std::vector<std::string>{"OnSpellAnnounced"},
        [&world](EventContext&) { world.focused = true; });
    auto empower = std::make_shared<ActionEvent>(world, "empower", std::vector<std::string>{"OnGatherDamage"},
        [](EventContext& ctx) { add_flat(ctx, 2); });
    // Only true because arcane_focus ran first; spending the focus fires backlash.
    auto focused_power = std::make_shared<ActionEvent>(world, "focused_power", std::vector<std::string>{"OnGatherDamage"},
        [&world](EventContext& ctx) { add_flat(ctx, 3); world.focused = false; },
        R"({"STATUS","focused"})", "OnFocusSpent");
    // Triggered by a modifier, so it gathers separately and must not see Fireball's payload.
    auto backlash = std::make_shared<DealDamage>(world, "backlash", std::vector<std::string>{"OnFocusSpent"}, "mage", 1.0f);
    auto afterglow = std::make_shared<ActionEvent>(world, "afterglow", std::vector<std::string>{"OnSpellResolved"});
    auto counterspell = std::make_shared<ActionEvent>(world, "counterspell", std::vector<std::string>{"OnFrostbolt"},
        [](EventContext& ctx) { ctx.cancel_definition("frostbolt"); });

    for (const auto& e : std::vector<std::shared_ptr<EventBase>>{arcane_focus, empower, focused_power, backlash, afterglow, counterspell})
        EventStore::register_event(e);

    auto fireball = std::make_shared<DealDamage>(world, "fireball", std::vector<std::string>{}, "goblin", 10.0f, "OnSpellAnnounced", "OnSpellResolved");
    auto frostbolt = std::make_shared<DealDamage>(world, "frostbolt", std::vector<std::string>{}, "goblin", 6.0f, "OnFrostbolt");

    EventQueue q;
    q.push_normal_event(QueuedEvent{fireball});
    q.push_normal_event(QueuedEvent{frostbolt});

    auto root = std::make_shared<EventContext>("Turn", "mage");
    EventQueueHandler handler;
    handler.process(q, root);

    REQUIRE(world.log == std::vector<std::string>{
        "arcane_focus",
        "empower",
        "focused_power",
        "empower",
        "backlash hits mage for 3",
        "fireball hits goblin for 15",
        "afterglow",
        "counterspell",
    });
    REQUIRE(world.hp["goblin"] == 35);
    REQUIRE(world.hp["mage"] == 47);

    REQUIRE(root->depth_tracker->current_depth == -1);
    REQUIRE_FALSE(root->is_definition_banned("fireball"));
    REQUIRE_FALSE(root->is_definition_banned("empower"));
    auto* cancellations = dynamic_cast<CancellationPayload*>(root->get_payload(CancellationPayload::KEY));
    REQUIRE(cancellations != nullptr);
    REQUIRE_FALSE(cancellations->is_cancelled("frostbolt", "frostbolt"));   // one-time cancel was used up
}
