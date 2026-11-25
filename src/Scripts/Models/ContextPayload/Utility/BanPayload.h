#pragma once

#include <string>
#include <unordered_set>
#include "Scripts/Models/ContextPayload/ContextPayload.h"

class BanPayload : public ContextPayload
{
public:
    static constexpr const char* KEY = "bans";

    std::unordered_set<std::string> banned_definition_ids {};

    std::string_view type_id() const override { return "BanPayload"; }
    PropagationMode propagation() const override { return PropagationMode::SharedReference; }

    bool merge_from(const ContextPayload& other) override;

    void ban(const std::string& definition_id);
    bool is_banned(const std::string& definition_id) const;
};
