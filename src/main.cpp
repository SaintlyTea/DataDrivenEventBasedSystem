#include "Scripts/Models/Events/CombatEvents/Effects/ApplyStatusEffect.h"
#include "Scripts/Models/EventContext/Combat/EventCombatContext.h"
#include "Scripts/Models/Expression.h"
#include "Scripts/Models/Entities/EventOwners/MockOwner.h"

#include <iostream>
#include <memory>

int main() {

    Expression::set_eval_atom([](const std::string&, const std::string&, const EventContext&) {
    return true;
});

    std::cout << "=== Smoke Test ===\n";

    EventCombatContext ctx("OnAttack", "unit_01", std::vector<std::string>{""});

    auto source = make_shared<MockOwner>("0", "0", std::vector<std::string>{"1"});
    auto owner = make_shared<MockOwner>("1", "1", std::vector<std::string>{"1"});
    std::unique_ptr<EventCombatBase> event = 
        std::make_unique<ApplyEffect>("burn_id", "Burn", source, owner, std::vector<std::string>{"OnAttack"}, "", "burn");
    std::cout << "Calling execute()...\n";
    event->execute(ctx);

    std::cout << "=== Done ===\n";
    return 0;
}