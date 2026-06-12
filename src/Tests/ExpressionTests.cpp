#include <catch2/catch_test_macros.hpp>
#include <unordered_map>
#include <string>

#include "Scripts/Models/Expression.h"
#include "Scripts/States/ExpressionStore.h"

// Controllable eval_atom for testing
// We define it here so tests can set which atoms return true or false
static std::unordered_map<std::string, bool> atom_results;

static struct ExpressionTestSetup {
    ExpressionTestSetup() {
        Expression::set_eval_atom([](const std::string& type, const std::string& value, const EventContext& ctx) {
            std::string key = type + ":" + value;
            auto it = atom_results.find(key);
            if (it != atom_results.end()) return it->second;
            return false;
        });
    }
} expression_test_setup;


// Helper to keep tests readable
static void set_atom(std::string type, std::string value, bool result)
{
    atom_results[type + ":" + value] = result;
}

static void clear_atoms()
{
    atom_results.clear();
    ExpressionStore::clear();
}

// Minimal context - content doesn't matter for expression tests
static EventContext make_ctx()
{
    return EventContext("test", "unit_01");
}

// ===========================
// Empty expression
// ===========================
TEST_CASE("Empty expression returns true", "[expression]")
{
    clear_atoms();
    auto expr = ExpressionStore::get_or_create("");
    REQUIRE(expr->evaluate(make_ctx()) == true);
}

// ===========================
// Single atom
// ===========================
TEST_CASE("Single atom - true", "[expression]")
{
    clear_atoms();
    set_atom("WEEKDAY", "MONDAY", true);
    auto expr = ExpressionStore::get_or_create("{\"WEEKDAY\",\"MONDAY\"}");
    REQUIRE(expr->evaluate(make_ctx()) == true);
}

TEST_CASE("Single atom - false", "[expression]")
{
    clear_atoms();
    set_atom("WEEKDAY", "MONDAY", false);
    auto expr = ExpressionStore::get_or_create("{\"WEEKDAY\",\"MONDAY\"}");
    REQUIRE(expr->evaluate(make_ctx()) == false);
}

// ===========================
// AND
// ===========================
TEST_CASE("AND - both true", "[expression]")
{
    clear_atoms();
    set_atom("A", "1", true);
    set_atom("B", "2", true);
    auto expr = ExpressionStore::get_or_create("{\"A\",\"1\"} * {\"B\",\"2\"}");
    REQUIRE(expr->evaluate(make_ctx()) == true);
}

TEST_CASE("AND - left false short circuits", "[expression]")
{
    clear_atoms();
    set_atom("A", "1", false);
    // B is not set - if short circuit works, it won't be evaluated
    // and we won't get a wrong result from the default false
    auto expr = ExpressionStore::get_or_create("{\"A\",\"1\"} * {\"B\",\"2\"}");
    REQUIRE(expr->evaluate(make_ctx()) == false);
}

TEST_CASE("AND - left true right false", "[expression]")
{
    clear_atoms();
    set_atom("A", "1", true);
    set_atom("B", "2", false);
    auto expr = ExpressionStore::get_or_create("{\"A\",\"1\"} * {\"B\",\"2\"}");
    REQUIRE(expr->evaluate(make_ctx()) == false);
}

// ===========================
// OR
// ===========================
TEST_CASE("OR - left true short circuits", "[expression]")
{
    clear_atoms();
    set_atom("A", "1", true);
    // B not set - if short circuit works, result is still true
    auto expr = ExpressionStore::get_or_create("{\"A\",\"1\"} + {\"B\",\"2\"}");
    REQUIRE(expr->evaluate(make_ctx()) == true);
}

TEST_CASE("OR - left false right true", "[expression]")
{
    clear_atoms();
    set_atom("A", "1", false);
    set_atom("B", "2", true);
    auto expr = ExpressionStore::get_or_create("{\"A\",\"1\"} + {\"B\",\"2\"}");
    REQUIRE(expr->evaluate(make_ctx()) == true);
}

TEST_CASE("OR - both false", "[expression]")
{
    clear_atoms();
    auto expr = ExpressionStore::get_or_create("{\"A\",\"1\"} + {\"B\",\"2\"}");
    REQUIRE(expr->evaluate(make_ctx()) == false);
}

// ===========================
// NOT
// ===========================
TEST_CASE("NOT - negates true to false", "[expression]")
{
    clear_atoms();
    set_atom("A", "1", true);
    auto expr = ExpressionStore::get_or_create("-{\"A\",\"1\"}");
    REQUIRE(expr->evaluate(make_ctx()) == false);
}

TEST_CASE("NOT - negates false to true", "[expression]")
{
    clear_atoms();
    auto expr = ExpressionStore::get_or_create("-{\"A\",\"1\"}");
    REQUIRE(expr->evaluate(make_ctx()) == true);
}

// ===========================
// Compound
// ===========================
TEST_CASE("Compound - (A AND B) OR C, only C true", "[expression]")
{
    clear_atoms();
    set_atom("C", "3", true);
    auto expr = ExpressionStore::get_or_create("({\"A\",\"1\"} * {\"B\",\"2\"}) + {\"C\",\"3\"}");
    REQUIRE(expr->evaluate(make_ctx()) == true);
}

TEST_CASE("Compound - NOT A AND B", "[expression]")
{
    clear_atoms();
    set_atom("A", "1", false);
    set_atom("B", "2", true);
    auto expr = ExpressionStore::get_or_create("-{\"A\",\"1\"} * {\"B\",\"2\"}");
    REQUIRE(expr->evaluate(make_ctx()) == true);
}