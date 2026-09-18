#include "hpps/AppTasks.hpp"
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 1024
#define PRIORITY_NORMAL 7
#define PRIORITY_HIGH 5

K_THREAD_STACK_DEFINE(test_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(worker_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(safety_stack, STACK_SIZE);

//----------------------------------------------------
// SELF_TEST_TASK
//----------------------------------------------------

SelfTestTask::SelfTestTask(MosfetController &ctrl)
    : mosfets(ctrl), tid(nullptr) {}

void SelfTestTask::threadEntry(void *p1, void *p2, void *p3) {
  auto *self = static_cast<SelfTestTask *>(p1);
  self->run();
}

int SelfTestTask::start() {
  tid = k_thread_create(&thread_data, test_stack,
                        K_THREAD_STACK_SIZEOF(test_stack), threadEntry, this,
                        nullptr, nullptr, PRIORITY_NORMAL, 0, K_NO_WAIT);
  return (tid != nullptr) ? 0 : -EFAULT;
}

void SelfTestTask::wait() {
  if (tid) {
    k_thread_join(tid, K_FOREVER);
  }
}

void SelfTestTask::run() {
  printk("[SelfTestTask] Calisiyor: 4 Kanal Sirayla Test Ediliyor...\n");

  MosfetChannel channels[] = {MosfetChannel::CH1, MosfetChannel::CH2,
                              MosfetChannel::CH3, MosfetChannel::CH4};

  for (int i = 0; i < 4; ++i) {
    printk(" -> Test ediliyor: Kanal %d\n", i + 1);
    mosfets.setDuty(channels[i], 100);
    k_msleep(1000);
    mosfets.setDuty(channels[i], 0);
    k_msleep(1000);
  }

  printk("[SelfTestTask] Test basariyla tamamlandi. Thread kapaniyor.\n");
}

//----------------------------------------------------
// MOSFET_WORKER_TASK
//----------------------------------------------------

MosfetWorkerTask::MosfetWorkerTask(MosfetController &ctrl)
    : mosfets(ctrl), tid(nullptr), is_running(true) {}

void MosfetWorkerTask::threadEntry(void *p1, void *p2, void *p3) {
  auto *self = static_cast<MosfetWorkerTask *>(p1);
  self->run();
}

int MosfetWorkerTask::start() {
  tid = k_thread_create(&thread_data, worker_stack,
                        K_THREAD_STACK_SIZEOF(worker_stack), threadEntry, this,
                        nullptr, nullptr, PRIORITY_NORMAL, 0, K_NO_WAIT);
  return (tid != nullptr) ? 0 : -EFAULT;
}

void MosfetWorkerTask::run() {
  printk("[MosfetWorkerTask] Ana calisma thread'i devrede!\n");

  uint8_t duty = 0;
  while (1) {
    if (is_running) {
      mosfets.setDuty(MosfetChannel::CH1, duty);
      mosfets.setDuty(MosfetChannel::CH2, 100 - duty);

      duty = (duty + 10) % 110;
    }

    k_msleep(200);
  }
}

//----------------------------------------------------
// SAFETY_TASK
//----------------------------------------------------

#define USER_BUTTON_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button =
    GPIO_DT_SPEC_GET(USER_BUTTON_NODE, gpios);
static SafetyTask *s_safety_instance = nullptr;

SafetyTask::SafetyTask(MosfetController &ctrl) : mosfets(ctrl), tid(nullptr) {
  k_sem_init(&trigger_sem, 0, 1);
}

void SafetyTask::isrCallback(const struct device *port,
                             struct gpio_callback *cb, uint32_t pins) {
  if (s_safety_instance) {
    k_sem_give(&s_safety_instance->trigger_sem);
  }
}

void SafetyTask::threadEntry(void *p1, void *p2, void *p3) {
  auto *self = static_cast<SafetyTask *>(p1);
  self->run();
}

int SafetyTask::init() {
  s_safety_instance = this;

  if (!gpio_is_ready_dt(&button))
    return -ENODEV;

  gpio_pin_configure_dt(&button, GPIO_INPUT);
  gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
  gpio_init_callback(&btn_cb_data, isrCallback, BIT(button.pin));
  return gpio_add_callback(button.port, &btn_cb_data);
}

int SafetyTask::start() {
  tid = k_thread_create(&thread_data, safety_stack,
                        K_THREAD_STACK_SIZEOF(safety_stack), threadEntry, this,
                        nullptr, nullptr, PRIORITY_HIGH, 0, K_NO_WAIT);
  return (tid != nullptr) ? 0 : -EFAULT;
}

void SafetyTask::run() {
  printk("[SafetyTask] Acil durum gozlemcisi hazir.\n");

  while (1) {
    // Butona basılana kadar bu thread UYUR (Sıfır CPU tüketimi)
    k_sem_take(&trigger_sem, K_FOREVER);

    printk("[SafetyTask] ACIL DURDURMA TETIKLENDI! Tum cikislar kesiliyor!\n");
    mosfets.allOff();
  }
}
