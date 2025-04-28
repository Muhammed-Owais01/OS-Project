#ifndef MESSAGE_PROCESSOR_H
#define MESSAGE_PROCESSOR_H

#include "message_queue.h"
#include "thread_safe_data.h"
#include "http_parser.h"
#include <stdbool.h>

typedef struct {
    MessageQueue* queue;
    ThreadSafeData* shared_data;
    bool running;
} MessageProcessor;

void message_processor_init(MessageProcessor* mp, MessageQueue* queue, ThreadSafeData* data);
void message_processor_start(MessageProcessor* mp);
void message_processor_stop(MessageProcessor* mp);

#endif // MESSAGE_PROCESSOR_H