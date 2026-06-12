#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Scripts/Models/ContextPayload/ContextPayload.h"

class DamageCalcData : public ContextPayload
{
public:
    float base    {0.0f};
    float flat    {0.0f};
    float percent {0.0f};
    int   final   {0};
    std::string        dtype {"phys"};
    std::vector<std::string> tags  {};

    const char* type_id() const override { return "DamageCalcData"; }

    std::unique_ptr<ContextPayload> merge_with(const ContextPayload& other) const override
    {
        if (other.type_id() != type_id()) 
            return nullptr; // incompatible types

        const auto& o = static_cast<const DamageCalcData&>(other);
        auto out = std::make_unique<DamageCalcData>();

        out->base    = (base != 0.0f) ? base : o.base;
        out->flat    = flat + o.flat;
        out->percent = percent + o.percent;
        out->dtype   = !dtype.empty() ? dtype : o.dtype;
        out->final   = o.final;
        out->tags    = tags;
        if (out->tags.empty() && !o.tags.empty())
            out->tags = o.tags;

        return out;
    }
};