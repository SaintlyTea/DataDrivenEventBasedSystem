#include "Scripts/Models/EventOpQueue.h"

void EventOpQueue::push_normal_op(EventBase* event)
{
    fifo_.push_back(event);
}

void EventOpQueue::push_barrier_op(EventBase* event)
{
    lifo_.push_back(event);
}

void EventOpQueue::enqueue_normal_ops(const EventOpQueue& batch)
{
    for (auto* event : batch.fifo_)
        fifo_.push_back(event);
}

void EventOpQueue::enqueue_barrier_ops(const EventOpQueue& batch)
{
    for (int i = batch.fifo_.size() - 1; i >= 0; --i)
        lifo_.push_back(batch.fifo_[i]);
}

EventBase* EventOpQueue::pop_next()
{
    if (!lifo_.empty())
    {
        EventBase* event = lifo_.back();
        lifo_.pop_back();
        return event;
    }
    if (!fifo_.empty())
    {
        EventBase* event = fifo_.front();
        fifo_.pop_front();
        return event;
    }
    throw std::runtime_error("pop_next called on empty queue");
}

bool EventOpQueue::is_empty() const
{
    return fifo_.empty() && lifo_.empty();
}

int EventOpQueue::fifo_size() const { return fifo_.size(); }
int EventOpQueue::lifo_size() const { return lifo_.size(); }