#include <3ds.h>
#include <string.h>
#include "plgldr.h"

static Handle       thread;
static Handle       onProcessExitEvent, resumeExitEvent;
static u8           stack[0x1000] __attribute__((aligned(8)));

static volatile int g_active = 0;
static volatile int g_cycle = 0;

static void waitMs(u32 ms) {
    svcSleepThread((u64)ms * 1000000);
}

static void pressA(void) {
    for (int i = 0; i < 5; i++) {
        hidTapKey(KEY_A);
        waitMs(300);
    }
}

static void farmCycle(void) {
    hidHoldKey(KEY_DRIGHT);
    waitMs(600);
    hidReleaseKey(KEY_DRIGHT);
    hidTapKey(KEY_A);
    waitMs(3000);
    hidHoldKey(KEY_DLEFT);
    waitMs(600);
    hidReleaseKey(KEY_DLEFT);
    hidTapKey(KEY_A);
    waitMs(3000);
}

static void ThreadMain(void *arg) {
    (void)arg;

    while (1) {
        if (svcWaitSynchronization(onProcessExitEvent, 50000000) != 0x09401BFE)
            goto exit;

        u32 state = hidSharedMem[6];

        if ((state & (KEY_L | KEY_R)) == (KEY_L | KEY_R)) {
            g_active = !g_active;
            if (g_active) {
                g_cycle = 0;
                PLGLDR__DisplayMessage("YKW2", "Started");
            } else {
                PLGLDR__DisplayMessage("YKW2", "Stopped");
            }
            waitMs(300);
        }

        if (g_active && g_cycle < 200) {
            farmCycle();
            g_cycle++;
        }
    }

exit:
    srvExit();
    svcSignalEvent(resumeExitEvent);
    svcExitThread();
}

extern char* fake_heap_start;
extern char* fake_heap_end;
extern u32 __ctru_heap;
extern u32 __ctru_linear_heap;

u32 __ctru_heap_size        = 0;
u32 __ctru_linear_heap_size = 0;

void __system_allocateHeaps(PluginHeader *header) {
    (void)header;
    __ctru_heap = 0;
    __ctru_linear_heap = 0;
    fake_heap_start = 0;
    fake_heap_end = 0;
}

void main(void) {
    srvInit();
    plgLdrInit();

    PLGLDR__DisplayMessage("YKW2", "Plugin loaded!");

    svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_GET_ON_EXIT_EVENT,
        (u32)&onProcessExitEvent, (u32)&resumeExitEvent);

    svcCreateThread(&thread, ThreadMain, 0,
        (u32 *)(stack + sizeof(stack)), 30, -1);
}

void *__service_ptr = NULL;
