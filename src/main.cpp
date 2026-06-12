#include "Scripts/Models/Events/CombatEvents/ApplyStatusEffect.h"
#include "Scripts/Models/EventContext/Combat/EventCombatContext.h"
#include "Scripts/Models/Expression.h"

#include <iostream>
#include <memory>

int main() {

    Expression::set_eval_atom([](const std::string& type, const std::string& value, const EventContext& ctx) {
    return true;
});

    std::cout << "=== Smoke Test ===\n";

    EventCombatContext ctx("OnAttack", "unit_01", std::vector<std::string>{""});

    std::unique_ptr<EventCombatBase> event = 
        std::make_unique<ApplyEffect>("burn_id", "Burn", "unit_01", "OnAttack", "", "burn");
    std::cout << "Calling execute()...\n";
    event->execute(ctx);

    std::cout << "=== Done ===\n";
    return 0;
}