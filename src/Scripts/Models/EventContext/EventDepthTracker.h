#pragma once

#include <unordered_map>
#include <algorithm>

struct EventDepthTracker
{
    int current_depth   {-1};
    int max_depth       {25};
    std::unordered_map<int, int> depth_visit_counts {};
    int   base_tolerance {100};
    float scale_factor   {0.2f};
    int   min_tolerance  {5};

    // Counts definition ids, so all instances of one ability share it: keep it generous.
    int   width_tolerance {40};

    // Runs a banned definition still gets in each handler other than the one that banned it.
    int   ban_allowance   {1};

    // Returns false and leaves depth unchanged when the entry is refused.
    bool enter_sub_handler();
    void exit_sub_handler();
    int  tolerance_at(int depth) const;
};