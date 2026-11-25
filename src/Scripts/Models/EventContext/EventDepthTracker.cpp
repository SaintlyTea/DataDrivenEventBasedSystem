#include "Scripts/Models/EventContext/EventDepthTracker.h"

bool EventDepthTracker::enter_sub_handler()
{
    if (current_depth >= max_depth) return false;
    current_depth++;
    depth_visit_counts[current_depth]++;
    if (depth_visit_counts[current_depth] <= tolerance_at(current_depth))
        return true;

    exit_sub_handler();
    return false;
}

void EventDepthTracker::exit_sub_handler()
{
    current_depth--;
    if (current_depth == -1)
        depth_visit_counts.clear();
}

int EventDepthTracker::tolerance_at(int depth) const
{
    float t = base_tolerance - (depth * depth * scale_factor);
    return std::max(static_cast<float>(min_tolerance), t);
}
