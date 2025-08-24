#include "audio.h"
#include "player.h"
#include "fastlz.h"
#include "callstack.h"
#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>

#ifdef RTOS_USE_THREADX
#include "pthread.h"
static void* pthread_entry(void* parameter) {
    int result = 0, counter = 0;
    struct timespec sleep = { 0,0 };
    main_audio();
    main_player();
    while (true) {
        sleep.tv_nsec = 999999999;
        sleep.tv_sec = 0;
        result = nanosleep(&sleep, 0);
        printf("%s:%d \n", __FUNCTION__, counter++);
        if (result)
            break;
    }
    return(&result);
}

void tx_application_define(void* first_unused_memory) {
    static pthread_t pthread = { NULL };
    static pthread_attr_t ptattr = { NULL };
    static uint8_t pheap[256 * 1024] = { NULL };
    struct sched_param parameter = { NULL };
    void* pmemery = posix_initialize(pheap);

    memset(&parameter, 0, sizeof(parameter));
    parameter.sched_priority = 10;
    pthread_attr_init(&ptattr);
    pthread_attr_setstackaddr(&ptattr, pmemery);
    pthread_attr_setschedparam(&ptattr, &parameter);
    pthread_create(&pthread, &ptattr, pthread_entry, NULL);
}

int main(void) {
    callstack();
    tx_kernel_enter();
    return 0;
}
#else //RTOS_USE_FREERTOS
void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    for (;;);
}

static void mainTask(void* parameters) {
    main_audio();
    main_player();
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void mainTaskInit(void) {
    static StaticTask_t mainTaskTCB;
    static StackType_t mainTaskStack[512];
    xTaskCreateStatic(mainTask, "main", 512, NULL, configMAX_PRIORITIES - 1U, mainTaskStack, &mainTaskTCB);
}

int main(void) {
    callstack();
    mainTaskInit();
    vTaskStartScheduler();
    return 0;
}
#endif