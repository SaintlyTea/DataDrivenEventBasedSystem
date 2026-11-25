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
    std::string dtype {"phys"};
    std::vector<std::string> tags {};

    std::string_view type_id() const override { return "DamageTakenData"; }
    PropagationMode propagation() const override { return PropagationMode::Gather; }
    bool merge_from(const ContextPayload& other) override;
};