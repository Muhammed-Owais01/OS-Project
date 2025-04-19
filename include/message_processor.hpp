#pragma once
#include "message_queue.hpp"
#include "thread_safe_data.hpp"

class MessageProcessor {
    MessageQueue& queue_;
    ThreadSafeData& shared_data_;
public:
    MessageProcessor(MessageQueue& queue, ThreadSafeData& data);
    void processMessages();
};