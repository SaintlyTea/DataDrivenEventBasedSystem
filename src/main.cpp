#include "Scripts/Models/Events/CombatEvents/ApplyStatusEffect.h"
#include "Scripts/Models/EventContext/Combat/EventCombatContext.h"
#include "Scripts/Models/Expression.h"

#include <iostream>
#include <memory>

// Temporary stub - we'll replace this with real game state later
int main() {

    Expression::set_eval_atom([](const std::string& type, const std::string& value, const EventContext& ctx) {
    return true;
});

    std::cout << "=== Smoke Test ===\n";

    // Build a minimal context
    EventCombatContext ctx("OnAttack", "unit_01", std::vector<std::string>{""});

    // Create event through base pointer - this is the key pattern
    std::unique_ptr<EventCombatBase> event = 
        std::make_unique<ApplyEffect>("burn_id", "Burn", "unit_01", "OnAttack", "", "burn");
    // Call execute() - should check conditions then call execute_() on the child
    std::cout << "Calling execute()...\n";
    event->execute(ctx);

    std::cout << "=== Done ===\n";
    return 0;
}