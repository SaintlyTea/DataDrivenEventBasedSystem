#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <catch2/catch_approx.hpp>

#include "Scripts/Models/EventContext/EventContext.h"
#include "Scripts/Models/ContextPayload/Combat/Damage/DamageCalcData.h"
#include "Scripts/Models/ContextPayload/Combat/Damage/DamageTakenData.h"

// ===========================
// DamageCalcData merge
// ===========================
TEST_CASE("DamageCalcData - flat and percent accumulate on merge", "[payload]")
{
    DamageCalcData a;
    a.flat = 10.0f; a.percent = 0.1f;

    DamageCalcData b;
    b.flat = 5.0f; b.percent = 0.2f;

    auto result = a.merge_with(b);
    auto& r = static_cast<DamageCalcData&>(*result);

    REQUIRE(r.flat    == 15.0f);
    REQUIRE(r.percent == Catch::Approx(0.3f));
}

TEST_CASE("DamageCalcData - existing base is kept if set", "[payload]")
{
    DamageCalcData a; a.base = 20.0f;
    DamageCalcData b; b.base = 10.0f;

    auto result = a.merge_with(b);
    auto& r = static_cast<DamageCalcData&>(*result);

    REQUIRE(r.base == 20.0f);
}

TEST_CASE("DamageCalcData - other base used if existing is zero", "[payload]")
{
    DamageCalcData a; a.base = 0.0f;
    DamageCalcData b; b.base = 10.0f;

    auto result = a.merge_with(b);
    auto& r = static_cast<DamageCalcData&>(*result);

    REQUIRE(r.base == 10.0f);
}

TEST_CASE("DamageCalcData - incompatible type returns nullptr", "[payload]")
{
    DamageCalcData a;
    DamageTakenData b;

    auto result = a.merge_with(b);
    REQUIRE(result == nullptr);
}

// ===========================
// DamageTakenData merge
// ===========================
TEST_CASE("DamageTakenData - flat and percent accumulate on merge", "[payload]")
{
    DamageTakenData a; a.flat = 3.0f; a.percent = 0.05f;
    DamageTakenData b; b.flat = 2.0f; b.percent = 0.10f;

    auto result = a.merge_with(b);
    auto& r = static_cast<DamageTakenData&>(*result);

    REQUIRE(r.flat    == 5.0f);
    REQUIRE(r.percent == Catch::Approx(0.15f));
}

TEST_CASE("DamageTakenData - incompatible type returns nullptr", "[payload]")
{
    DamageTakenData a;
    DamageCalcData b;

    auto result = a.merge_with(b);
    REQUIRE(result == nullptr);
}

// ===========================
// EventContext
// ===========================
TEST_CASE("EventContext - has_payload false before set", "[context]")
{
    EventContext ctx("OnAttack", "unit_01");
    REQUIRE(ctx.has_payload("damage") == false);
}

TEST_CASE("EventContext - set and get payload", "[context]")
{
    EventContext ctx("OnAttack", "unit_01");

    auto data = std::make_unique<DamageCalcData>();
    data->flat = 10.0f;
    ctx.set_payload("damage", std::move(data));

    REQUIRE(ctx.has_payload("damage") == true);

    auto* result = static_cast<DamageCalcData*>(ctx.get_payload("damage"));
    REQUIRE(result->flat == 10.0f);
}

TEST_CASE("EventContext - get_payload returns nullptr for missing key", "[context]")
{
    EventContext ctx("OnAttack", "unit_01");
    REQUIRE(ctx.get_payload("damage") == nullptr);
}

TEST_CASE("EventContext - set_payload twice merges values", "[context]")
{
    EventContext ctx("OnAttack", "unit_01");

    auto a = std::make_unique<DamageCalcData>(); a->flat = 10.0f;
    auto b = std::make_unique<DamageCalcData>(); b->flat = 5.0f;

    ctx.set_payload("damage", std::move(a));
    ctx.set_payload("damage", std::move(b));

    auto* result = static_cast<DamageCalcData*>(ctx.get_payload("damage"));
    REQUIRE(result->flat == 15.0f);
}

TEST_CASE("EventContext - incompatible merge keeps existing payload unchanged", "[context]")
{
    EventContext ctx("OnAttack", "unit_01");

    auto a = std::make_unique<DamageCalcData>(); a->flat = 10.0f;
    auto b = std::make_unique<DamageTakenData>(); b->flat = 99.0f;

    ctx.set_payload("damage", std::move(a));
    ctx.set_payload("damage", std::move(b)); // incompatible - should be ignored

    auto* result = static_cast<DamageCalcData*>(ctx.get_payload("damage"));
    REQUIRE(result->flat == 10.0f); // unchanged
}