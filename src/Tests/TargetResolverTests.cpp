#include <catch2/catch_test_macros.hpp>
#include <memory>

// Both headers in one translation unit: TargetingStrategy must be declared only once.
#include "Scripts/Constants/TargetingStrategy.h"
#include "Scripts/Handlers/Targeting/TargetResolver.h"
#include "Scripts/Models/EventContext/Combat/EventCombatContext.h"

TEST_CASE("TargetResolver - SELF targets the owner without explicit targets", "[targeting]")
{
    auto ctx = std::make_shared<EventContext>("test", "unit_01");
    REQUIRE(TargetResolver::resolve(TargetingStrategy::SELF, "owner_01", ctx) ==
            std::vector<std::string>{"owner_01"});
}

TEST_CASE("TargetResolver - EXPLICIT without explicit targets is empty", "[targeting]")
{
    auto ctx = std::make_shared<EventContext>("test", "unit_01");
    REQUIRE(TargetResolver::resolve(TargetingStrategy::EXPLICIT, "owner_01", ctx).empty());
}

TEST_CASE("TargetResolver - EXPLICIT returns the given targets", "[targeting]")
{
    auto ctx = std::make_shared<EventContext>("test", "unit_01");
    REQUIRE(TargetResolver::resolve(TargetingStrategy::EXPLICIT, "owner_01", ctx, {"t1", "t2"}) ==
            std::vector<std::string>{"t1", "t2"});
}

TEST_CASE("TargetResolver - INHERIT copies a combat context's targets", "[targeting]")
{
    std::shared_ptr<EventContext> ctx =
        std::make_shared<EventCombatContext>("test", "unit_01", std::vector<std::string>{"t1"});
    REQUIRE(TargetResolver::resolve(TargetingStrategy::INHERIT, "owner_01", ctx) ==
            std::vector<std::string>{"t1"});
}

TEST_CASE("TargetResolver - INHERIT from a non-combat context is empty", "[targeting]")
{
    auto ctx = std::make_shared<EventContext>("test", "unit_01");
    REQUIRE(TargetResolver::resolve(TargetingStrategy::INHERIT, "owner_01", ctx).empty());
}
