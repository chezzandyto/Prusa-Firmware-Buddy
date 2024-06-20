#include <logging/log_task.hpp>

#include <array>
#include <cstdio>
#include <cstdlib>

namespace logging {

Task::Task()
    : task_handle { nullptr } {
    TaskHandle_t local_task_handle = xTaskCreateStatic(
        +[](void *ctx) { static_cast<Task *>(ctx)->run(); },
        "log_task",
        STACK_SIZE,
        this,
        tskIDLE_PRIORITY + 2,
        task_stack,
        &task_buffer);
    if (local_task_handle == nullptr) {
        abort(); // Since we are allocating statically, this should never happen.
    }
}

void Task::run() {
    task_handle.store(xTaskGetCurrentTaskHandle());
    for (;;) {
        QueueItem item;
        queue.receive(item);

        // Format message as soon as possible to unblock semaphore holder.
        // Also, formats the string only once.
        // Also, saves stack by not requiring every task to have a buffer.
        Event *event = item.event;
        std::array<char, MESSAGE_MAX_SIZE> message;
        vsnprintf(message.data(), message.size(), event->fmt, *event->args);
        FormattedEvent formatted_event {
            .timestamp = event->timestamp, // OK to copy value
            .task_id = event->task_id, // OK to copy value
            .component = event->component, // OK to copy const* to static variable
            .severity = event->severity, // OK to copy value
            .message = message.data(), // OK to copy const* to local variable of this task, we will be blocking before it goes out of scope
        };
        item.semaphore->release(); // unblock, we no longer need anything from the event

        // Proceed with (potentially blocking) event processing.
        log_task_process_event(&formatted_event);
    }
}

void Task::send(Event *event) {
    if (xPortIsInsideInterrupt()) {
        // Note: We do not support logging from the interrupt handler.
        //       It is usually a bad idea anyway.
        // FIXME: But perhaps we can at least log via RTT.
        return;
    }
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        // Note: We require FreeRTOS scheduler to be running in order to drain
        //       the logging queue. If the scheduler was not yet started
        //       (i.e. shortly after boot) we discard the message, otherwise
        //       we would block on semaphore and never wake up.
        // FIXME: But perhaps we can at least log via RTT in such case.
        return;
    }
    if (xTaskGetCurrentTaskHandle() == task_handle) {
        // FIXME: For now, we do not support logging from the logging thread.
        //        In order to support that, we would have to support non-blocking
        //        insertion to the queue, which would in turn require solving the
        //        lifetime issues. We would also need to disable the semaphore.
        return;
    }
    if (task_handle == nullptr) {
        // Note: This fixes rare race condition when some task is trying to log
        //       before the logging task is up. In order to prevent deadlock,
        //       we throw away the message.
        return;
    }

    freertos::BinarySemaphore semaphore;
    QueueItem item {
        .event = event,
        .semaphore = &semaphore,
    };
    // We can't afford to block here, it might lead to deadlock. Try waiting
    // a bit and then just discard the message.
    if (queue.try_send(item, 10)) {
        // Block until logging task wakes us. We need this to keep the pointers
        // inside queue item alive. The logging task only needs them for short
        // time to format the message, which does neither log nor block.
        semaphore.acquire();
    }
}

} // namespace logging
