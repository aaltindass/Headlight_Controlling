#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "hpps/AppTasks.hpp"
#include "hpps/MosfetController.hpp"

static MosfetController mosfets;
static SelfTestTask     testTask(mosfets);
static MosfetWorkerTask workerTask(mosfets);
static SafetyTask       safetyTask(mosfets);

int main(void)
{
    printk("\n==========================================\n");
    printk("   STM32H563ZI RTOS Mosfet Supervisor     \n");
    printk("==========================================\n");

    // 1. ADIM: Donanımı hazırla
    if (mosfets.init() < 0 || safetyTask.init() < 0) {
        printk("Donanim ilklendirme basarisiz!\n");
        return -1;
    }

    // 2. ADIM (1. Sıra): Test thread'ini başlat ve BİTMESİNİ BEKLE
    printk("[MAIN] 1. Asama: Donanim testi baslatiliyor...\n");
    testTask.start();
    testTask.wait(); // k_thread_join: Test tamamlanana kadar kod burada durur

    // 3. ADIM (2. Sıra): Test bitti; Emniyet ve Ana Çalışma Thread'lerini başlat
    printk("[MAIN] 2. Asama: Normal operasyon thread'leri devreye aliniyor...\n");
    safetyTask.start();
    workerTask.start();

    printk("[MAIN] Tum thread'ler sirayla calistirildi. Main gorevini tamamladi.\n");

    // Main thread'in işi bitti; kendini sonsuz uykuya alabilir veya durum raporlayabilir
    while (1) {
        k_sleep(K_FOREVER);
    }

    return 0;
}
