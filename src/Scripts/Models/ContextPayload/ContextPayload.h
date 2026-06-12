#pragma once
#include <memory>

class ContextPayload
{
public:
    virtual ~ContextPayload() = default;

    virtual std::unique_ptr<ContextPayload> merge_with(const ContextPayload& other) const = 0;

    // Used to safely check type compatibility in merge_with
    virtual const char* type_id() const = 0;
};