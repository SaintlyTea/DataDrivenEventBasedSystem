#include "Scripts/Models/Events/CombatEvents/Effects/ApplyStatusEffect.h"

#include <iostream>

void ApplyEffect::execute_combat_(EventCombatContext& ctx)
{
    std::cout << "[ApplyEffect] '" << get_name()
              << "' applying status '" << status_id
              << "' to " << ctx.targets.size() << " target(s)\n";
}
