#pragma once

#include <deque>
#include <vector>
#include <stdexcept>

#include "Scripts/Models/Events/EventBase.h"
#include "Scripts/Models/EventQueue/QueuedEvent.h"

class EventQueue
{
private:
    std::deque<QueuedEvent> fifo_;
    std::vector<QueuedEvent> lifo_;

public:
    void push_normal_event(QueuedEvent event);
    void push_barrier_event(QueuedEvent event);

    void enqueue_normal_events(const EventQueue& batch);
    void enqueue_barrier_events(const EventQueue& batch);

    QueuedEvent pop_next();

    bool is_empty() const;
    int fifo_size() const;
    int lifo_size() const;
};