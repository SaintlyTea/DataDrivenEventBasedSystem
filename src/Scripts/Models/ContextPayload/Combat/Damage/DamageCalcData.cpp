#include "Scripts/Models/ContextPayload/Combat/Damage/DamageCalcData.h"

bool DamageCalcData::merge_from(const ContextPayload& other)
{
    if (other.type_id() != type_id()) return false;

    const auto& o = static_cast<const DamageCalcData&>(other);

    if (base == 0.0f) base = o.base;
    flat    += o.flat;
    percent += o.percent;
    if (dtype.empty()) dtype = o.dtype;
    final    = o.final;
    if (tags.empty()) tags = o.tags;

    return true;
}
