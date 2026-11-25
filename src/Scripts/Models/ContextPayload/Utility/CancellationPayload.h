#pragma once

#include <unordered_map>
#include <string>
#include "Scripts/Models/ContextPayload/ContextPayload.h"

class CancellationPayload : public ContextPayload
{
public:
    static constexpr const char* KEY = "cancellations";

    // id -> true: permanent, false: one-time
    std::unordered_map<std::string, bool> by_instance {};
    std::unordered_map<std::string, bool> by_definition {};

    std::string_view type_id() const override { return "CancellationPayload"; }
    PropagationMode propagation() const override { return PropagationMode::SharedReference; }

    bool merge_from(const ContextPayload& other) override;

    void cancel_instance(const std::string& instance_id, bool permanent = false);
    void cancel_definition(const std::string& definition_id, bool permanent = false);

    bool is_cancelled(const std::string& definition_id, const std::string& instance_id) const;

    // The instance entry is checked (and used) before the definition entry.
    bool check_and_consume(const std::string& definition_id, const std::string& instance_id);
};
