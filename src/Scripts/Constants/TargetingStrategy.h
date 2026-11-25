#pragma once

enum class TargetingStrategy {
    EXPLICIT,   // targets provided directly (e.g. player choice)
    INHERIT,    // use targets from the triggering context
    SELF,       // target the owner
    // AUTO_* strategies come later
};