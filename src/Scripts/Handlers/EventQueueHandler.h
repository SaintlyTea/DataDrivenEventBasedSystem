#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "Scripts/Models/EventQueue/EventQueue.h"
#include "Scripts/Models/EventContext/EventContext.h"

class EventQueueHandler
{
public:
    void process(EventQueue& queue, const std::shared_ptr<EventContext>& ctx);

private:
    // Lookup only: iteration order must never affect execution.
    std::unordered_map<std::string, int> executions_ {};
    std::unordered_set<std::string> banned_here_ {};
    std::unordered_map<std::string, int> ban_runs_ {};
    std::unordered_map<std::string, int> announcements_ {};

    bool may_execute_(const std::string& definition_id, EventContext& ctx);
    bool may_announce_(const std::string& definition_id, EventContext& ctx);
    void ban_here_(const std::string& definition_id, EventContext& ctx);
};
