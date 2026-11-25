#include "Scripts/Models/ContextPayload/Utility/BanPayload.h"

bool BanPayload::merge_from(const ContextPayload& other)
{
    if (other.type_id() != type_id()) return false;

    const auto& o = static_cast<const BanPayload&>(other);
    banned_definition_ids.insert(o.banned_definition_ids.begin(), o.banned_definition_ids.end());
    return true;
}

void BanPayload::ban(const std::string& definition_id)
{
    banned_definition_ids.insert(definition_id);
}

bool BanPayload::is_banned(const std::string& definition_id) const
{
    return banned_definition_ids.contains(definition_id);
}
