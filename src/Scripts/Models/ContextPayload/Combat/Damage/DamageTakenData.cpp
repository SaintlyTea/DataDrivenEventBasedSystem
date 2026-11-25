#include "Scripts/Models/ContextPayload/Combat/Damage/DamageTakenData.h"

bool DamageTakenData::merge_from(const ContextPayload& other)
{
    if (other.type_id() != type_id()) return false;

    const auto& o = static_cast<const DamageTakenData&>(other);

    if (base_def == 0.0f) base_def = o.base_def;
    flat    += o.flat;
    percent += o.percent;
    if (dtype.empty()) dtype = o.dtype;
    if (tags.empty()) tags = o.tags;

    return true;
}
