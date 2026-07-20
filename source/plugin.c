#include <3ds.h>
#include "plgldr.h"

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
}

void *__service_ptr = NULL;
