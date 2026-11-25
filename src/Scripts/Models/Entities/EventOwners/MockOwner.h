#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "Scripts/Models/Entities/EventOwners/IEventOwner.h"

class MockOwner : public IEventOwner
{
public:
    std::string definition_id;
    std::string instance_id;
    std::vector<std::string> registered_event_ids;

    MockOwner(std::string def_id, std::string inst_id, std::vector<std::string> pRegistered_event_ids)
        : definition_id(def_id), instance_id(inst_id), registered_event_ids(pRegistered_event_ids)
    {}

    std::string get_definition_id() const override { return definition_id; }
    std::string get_instance_id() const override { return instance_id; }

    int get_event_registration_order(const std::string& event_instance_id) const override
    {
        auto it = std::find(registered_event_ids.begin(), registered_event_ids.end(), event_instance_id);
        if (it == registered_event_ids.end()) return -1;
        return static_cast<int>(std::distance(registered_event_ids.begin(), it));
    }
};