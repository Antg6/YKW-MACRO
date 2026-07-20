#include <3ds.h>
#include <string.h>
#include "plgldr.h"

static Handle       thread;
static Handle       onProcessExitEvent, resumeExitEvent;
static u8           stack[0x1000] __attribute__((aligned(8)));

static volatile int g_active = 0;
static vu32 *ourHidMem = NULL;

static Result mapHid(void) {
    Handle h;
    Result rc = srvGetServiceHandle(&h, "hid:USER");
    if (R_FAILED(rc)) return rc;

    u32 *c = getThreadCommandBuffer();
    c[0] = IPC_MakeHeader(0xA, 0, 0);
    rc = svcSendSyncRequest(h);
    if (R_FAILED(rc)) return rc;

    Handle memHandle = c[3];
    static vu32 buf[0x2b0 / 4] __attribute__((aligned(0x1000)));
    rc = svcMapMemoryBlock(memHandle, (u32)buf, MEMPERM_READ, MEMPERM_DONTCARE);
    if (R_FAILED(rc)) return rc;

    ourHidMem = buf;
    return 0;
}

static void ThreadMain(void *arg) {
    (void)arg;

    while (1) {
        if (svcWaitSynchronization(onProcessExitEvent, 50000000) != 0x09401BFE)
            goto exit;

        if (ourHidMem) {
            u32 state = ourHidMem[6];
            if ((state & 0x0300) == 0x0300) {
                g_active = !g_active;
                PLGLDR__DisplayMessage("YKW2", g_active ? "ON" : "OFF");
                svcSleepThread(500000000);
            }
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

    PLGLDR__DisplayMessage("YKW2", "Loaded!");

    mapHid();

    svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_GET_ON_EXIT_EVENT,
        (u32)&onProcessExitEvent, (u32)&resumeExitEvent);

    svcCreateThread(&thread, ThreadMain, 0,
        (u32 *)(stack + sizeof(stack)), 30, -1);
}

void *__service_ptr = NULL;
