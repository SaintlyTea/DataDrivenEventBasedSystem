#pragma once
#include <string>

class IEventOwner
{
public:
    virtual ~IEventOwner() = default;

    virtual std::string get_definition_id() const = 0;
    virtual std::string get_instance_id() const = 0;
    virtual int get_event_registration_order(const std::string& event_instance_id) const = 0;
};