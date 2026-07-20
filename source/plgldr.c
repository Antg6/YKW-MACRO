#include <3ds.h>
#include <string.h>
#include "plgldr.h"

static Handle   plgLdrHandle;
static int      plgLdrRefCount;

Result plgLdrInit(void)
{
    Result res = 0;
    if (AtomicPostIncrement(&plgLdrRefCount) == 0)
        res = svcConnectToPort(&plgLdrHandle, "plg:ldr");
    if (R_FAILED(res))
        AtomicDecrement(&plgLdrRefCount);
    return res;
}

void plgLdrExit(void)
{
    if (AtomicDecrement(&plgLdrRefCount))
        return;
    svcCloseHandle(plgLdrHandle);
}

Result PLGLDR__IsPluginLoaderEnabled(bool *isEnabled)
{
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = IPC_MakeHeader(2, 0, 0);
    Result res = svcSendSyncRequest(plgLdrHandle);
    if (R_SUCCEEDED(res)) { res = cmdbuf[1]; *isEnabled = cmdbuf[2]; }
    return res;
}

Result PLGLDR__DisplayMessage(const char *title, const char *body)
{
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = IPC_MakeHeader(6, 0, 4);
    cmdbuf[1] = IPC_Desc_Buffer(strlen(title), IPC_BUFFER_R);
    cmdbuf[2] = (u32)title;
    cmdbuf[3] = IPC_Desc_Buffer(strlen(body), IPC_BUFFER_R);
    cmdbuf[4] = (u32)body;
    Result res = svcSendSyncRequest(plgLdrHandle);
    if (R_SUCCEEDED(res)) res = cmdbuf[1];
    return res;
}
