.arm
.align 4

.section .text

.macro SVC_BEGIN name
    .global \name
    .type \name, %function
    \name:
.endm

SVC_BEGIN svcCustomBackdoor
    push {r0-r3, r4, lr}
    ldr r4, [sp, #0x1C]
    ldr r3, =0xE920F001
    svc 0x7F
    pop {r0-r3, r4, pc}

SVC_BEGIN svcConvertVAToPA
    push {r1, lr}
    ldr r1, [sp, #8]
    svc 0x7F
    pop {r1, pc}

SVC_BEGIN svcFlushDataCacheRange
    svc 0x7F
    bx lr

SVC_BEGIN svcFlushEntireDataCache
    svc 0x7F
    bx lr

SVC_BEGIN svcInvalidateInstructionCacheRange
    svc 0x7F
    bx lr

SVC_BEGIN svcInvalidateEntireInstructionCache
    svc 0x7F
    bx lr

SVC_BEGIN svcMapProcessMemoryEx
    svc 0x7F
    bx lr

SVC_BEGIN svcUnmapProcessMemoryEx
    svc 0x7F
    bx lr

SVC_BEGIN svcControlMemoryEx
    svc 0x7F
    bx lr

SVC_BEGIN svcControlService
    svc 0x7F
    bx lr

SVC_BEGIN svcCopyHandle
    svc 0x7F
    bx lr

SVC_BEGIN svcTranslateHandle
    svc 0x7F
    bx lr

SVC_BEGIN svcControlProcess
    svc 0x7F
    bx lr
