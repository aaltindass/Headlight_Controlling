#pragma once

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "MosfetController.hpp"

class SelfTestTask {
    public:
        explicit SelfTestTask(MosfetController &ctrl);

        int start();
        void wait();

    private:
        MosfetController &mosfets;
        struct k_thread thread_data;
        k_tid_t tid;

        static void threadEntry(void *p1, void *p2, void *p3);
        void run();
};

class MosfetWorkerTask {
    public:
        explicit MosfetWorkerTask(MosfetController &ctrl);

        int start();
        void pause();
        void resume();

    private:
        MosfetController &mosfets;
        struct k_thread thread_data;
        k_tid_t tid;
        volatile bool is_running;

        static void threadEntry(void *p1, void *p2, void *p3);
        void run();
};

class SafetyTask {
    public:
        explicit SafetyTask(MosfetController &ctrl);

        int init();
        int start();

    private:
        MosfetController &mosfets;
        struct k_thread thread_data;
        k_tid_t tid;
        struct k_sem trigger_sem;
        struct gpio_callback btn_cb_data;

        static void threadEntry(void *p1, void *p2, void *p3);
        static void isrCallback(const struct device *port, struct gpio_callback *cb, uint32_t pins);
        void run();
        
};
