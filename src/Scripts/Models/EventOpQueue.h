#pragma once

#include <deque>
#include <vector>
#include <stdexcept>

#include "Scripts/Models/Events/EventBase.h"

class EventOpQueue
{
private:
    std::deque<EventBase*> fifo_;
    std::vector<EventBase*> lifo_;

public:
    void push_normal_op(EventBase* event);
    void push_barrier_op(EventBase* event);

    void enqueue_normal_ops(const EventOpQueue& batch);
    void enqueue_barrier_ops(const EventOpQueue& batch);

    EventBase* pop_next();

    bool is_empty() const;
    int fifo_size() const;
    int lifo_size() const;
};