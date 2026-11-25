#include "Scripts/Models/ContextPayload/Utility/CancellationPayload.h"

namespace
{
    // Permanent is never downgraded to one-time.
    void add_entry(std::unordered_map<std::string, bool>& entries, const std::string& id, bool permanent)
    {
        auto [it, inserted] = entries.try_emplace(id, permanent);
        if (!inserted && permanent)
            it->second = true;
    }

    bool use_entry(std::unordered_map<std::string, bool>& entries, const std::string& id)
    {
        auto it = entries.find(id);
        if (it == entries.end()) return false;
        if (!it->second)
            entries.erase(it);
        return true;
    }
}

bool CancellationPayload::merge_from(const ContextPayload& other)
{
    if (other.type_id() != type_id()) return false;

    const auto& o = static_cast<const CancellationPayload&>(other);
    for (const auto& [id, permanent] : o.by_instance)
        add_entry(by_instance, id, permanent);
    for (const auto& [id, permanent] : o.by_definition)
        add_entry(by_definition, id, permanent);
    return true;
}

void CancellationPayload::cancel_instance(const std::string& instance_id, bool permanent)
{
    add_entry(by_instance, instance_id, permanent);
}

void CancellationPayload::cancel_definition(const std::string& definition_id, bool permanent)
{
    add_entry(by_definition, definition_id, permanent);
}

bool CancellationPayload::is_cancelled(const std::string& definition_id, const std::string& instance_id) const
{
    return by_instance.contains(instance_id) || by_definition.contains(definition_id);
}

bool CancellationPayload::check_and_consume(const std::string& definition_id, const std::string& instance_id)
{
    return use_entry(by_instance, instance_id) || use_entry(by_definition, definition_id);
}
