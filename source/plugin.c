#include <3ds.h>
#include <string.h>
#include "plgldr.h"
#include "csvc.h"
#include "common.h"
#include "ykw2_farm.h"

static Handle       thread;
static Handle       onProcessExitEvent, resumeExitEvent;
static u8           stack[0x1000] __attribute__((aligned(8)));

static volatile int g_active = 0;
static volatile int g_cycle = 0;

static Handle hidHandle;
Handle hidMemHandle;
vu32 *hidSharedMem;
static vu32 ourSharedMem[0x2b0 / 4] __attribute__((aligned(0x1000)));

static Result initHid(void) {
    Result rc = srvGetServiceHandle(&hidHandle, "hid:USER");
    if (R_FAILED(rc)) return rc;

    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = IPC_MakeHeader(0xA, 0, 0);
    rc = svcSendSyncRequest(hidHandle);
    if (R_FAILED(rc)) return rc;

    hidMemHandle = cmdbuf[3];

    rc = svcMapMemoryBlock(hidMemHandle, (u32)ourSharedMem,
        MEMPERM_READ | MEMPERM_WRITE, MEMPERM_DONTCARE);
    if (R_FAILED(rc)) return rc;

    hidSharedMem = ourSharedMem;
    return 0;
}

static void inject(u32 buttons, s16 cpadX, s16 cpadY) {
    hidSharedMem[6] = buttons;
    hidSharedMem[7] = (u32)(u16)(cpadX + 0x800) | ((u32)(u16)(cpadY + 0x800) << 16);

    u32 cpad = (u32)(u16)cpadX | ((u32)(u16)cpadY << 16);
    for (int i = 0; i < 8; i++) {
        hidSharedMem[10 + i * 4] = buttons;
        hidSharedMem[10 + i * 4 + 3] = cpad;
    }
}

static void injectButtons(u32 buttons) {
    inject(buttons, 0, 0);
}

static void injectCpad(u32 buttons, s16 x, s16 y) {
    inject(buttons, x, y);
}

static void waitMs(u32 ms) {
    svcSleepThread((u64)ms * 1000000);
}

static void holdButtons(u32 buttons, u32 durationMs) {
    u32 elapsed = 0;
    while (elapsed < durationMs) {
        injectButtons(buttons);
        svcSleepThread(5000000);
        elapsed += 5;
    }
    injectButtons(0);
    svcSleepThread(50000000);
}

static void holdCpad(u32 buttons, s16 x, s16 y, u32 durationMs) {
    u32 elapsed = 0;
    while (elapsed < durationMs) {
        injectCpad(buttons, x, y);
        svcSleepThread(5000000);
        elapsed += 5;
    }
    injectButtons(0);
    svcSleepThread(50000000);
}

static void pressA(void) {
    for (int i = 0; i < COLLECT_TAPS; i++) {
        injectButtons(KEY_A);
        waitMs(50);
        injectButtons(0);
        waitMs(COLLECT_GAP_MS - 50);
    }
}

static void farmCycle(void) {
    holdCpad(KEY_CPAD_RIGHT, 200, 0, WALK_RIGHT_MS);
    pressA();
    holdCpad(KEY_CPAD_UP, 0, 200, WALK_UP_MS);
    pressA();
    holdCpad(KEY_CPAD_LEFT, -200, 0, WALK_LEFT_MS);
    pressA();
    holdCpad(KEY_CPAD_DOWN, 0, -200, WALK_DOWN_MS);
    pressA();
    holdButtons(KEY_DLEFT, 600);
    injectButtons(KEY_A);
    waitMs(ZONE_EXIT_MS);
    injectButtons(0);
    holdButtons(KEY_DRIGHT, 500);
    injectButtons(KEY_A);
    waitMs(ZONE_ENTER_MS);
    injectButtons(0);
}

static void ThreadMain(void *arg) {
    (void)arg;

    while (1) {
        if (svcWaitSynchronization(onProcessExitEvent, 50000000) != 0x09401BFE)
            goto exit;

        if (hidSharedMem) {
            u32 state = hidSharedMem[6];

            if ((state & (KEY_L | KEY_R)) == (KEY_L | KEY_R)) {
                g_active = !g_active;
                if (g_active) g_cycle = 0;
                waitMs(300);
            }

            if (g_active && g_cycle < MAX_CYCLES) {
                farmCycle();
                g_cycle++;
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
    initHid();

    svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_GET_ON_EXIT_EVENT,
        (u32)&onProcessExitEvent, (u32)&resumeExitEvent);

    svcCreateThread(&thread, ThreadMain, 0,
        (u32 *)(stack + sizeof(stack)), 30, -1);
}

void *__service_ptr = NULL;
