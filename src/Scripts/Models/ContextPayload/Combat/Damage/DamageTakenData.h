#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Scripts/Models/ContextPayload/ContextPayload.h"

class DamageTakenData : public ContextPayload
{
public:
    float base_def {0.0f};
    float flat     {0.0f};
    float percent  {0.0f};
    std::string        dtype {"phys"};
    std::vector<std::string> tags  {};

    const char* type_id() const override { return "DamageTakenData"; }

    std::unique_ptr<ContextPayload> merge_with(const ContextPayload& other) const override
    {
        if (other.type_id() != type_id()) 
            return nullptr;

        const auto& o = static_cast<const DamageTakenData&>(other);
        auto out = std::make_unique<DamageTakenData>();

        out->base_def = (base_def != 0.0f) ? base_def : o.base_def;
        out->flat     = flat + o.flat;
        out->percent  = percent + o.percent;
        out->dtype    = !dtype.empty() ? dtype : o.dtype;
        out->tags     = tags;
        if (out->tags.empty() && !o.tags.empty())
            out->tags = o.tags;

        return out;
    }
};