#include "Scripts/Models/EventQueue/EventQueue.h"

void EventQueue::push_normal_event(QueuedEvent event)
{
    fifo_.push_back(std::move(event));
}

void EventQueue::push_barrier_event(QueuedEvent event)
{
    lifo_.push_back(std::move(event));
}

void EventQueue::enqueue_normal_events(const EventQueue& batch)
{
    for (auto event : batch.fifo_)
        fifo_.push_back(event);
}

void EventQueue::enqueue_barrier_events(const EventQueue& batch)
{
    for (int i = batch.fifo_.size() - 1; i >= 0; --i)
        lifo_.push_back(batch.fifo_[i]);
}

QueuedEvent EventQueue::pop_next()
{
    if (!lifo_.empty())
    {
        QueuedEvent event = std::move(lifo_.back());
        lifo_.pop_back();
        return event;
    }
    if (!fifo_.empty())
    {
        QueuedEvent event = std::move(fifo_.front());
        fifo_.pop_front();
        return event;
    }
    throw std::runtime_error("pop_next called on empty queue");
}

bool EventQueue::is_empty() const
{
    return fifo_.empty() && lifo_.empty();
}

int EventQueue::fifo_size() const { return fifo_.size(); }
int EventQueue::lifo_size() const { return lifo_.size(); }