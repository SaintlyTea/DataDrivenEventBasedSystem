#pragma once
#include <memory>
#include <string_view>

enum class PropagationMode
{
    None,
    SharedReference,
    Copy,
    Gather // one hop: shared only with the events of a gathering sub-run
};

class ContextPayload
{
public:
    virtual ~ContextPayload() = default;

    // Returns false, leaving this unchanged, if the types differ.
    virtual bool merge_from(const ContextPayload& other) = 0;
    virtual std::string_view type_id() const = 0;

    virtual PropagationMode propagation() const { return PropagationMode::None; }
    // Required only for PropagationMode::Copy.
    virtual std::unique_ptr<ContextPayload> clone() const { return nullptr; }
};
