.arm
.align(4);
.section .text
.global _start
_start:
    stmfd   sp!, {r0-r12, lr}
    mrs     r0, cpsr
    stmfd   sp!, {r0}

    ldr     r0, = __c_bss_start
    ldr     r1, = __c_bss_end
    sub     r1, r1, r0
    bl      ClearMem

    bl      main

    ldmfd   sp!, {r0}
    msr     cpsr, r0
    ldmfd   sp!, {r0-r12, pc}

ClearMem:
    mov     r2, #3
    add     r1, r1, r2
    bics    r1, r1, r2
    bxeq    lr
    mov     r2, #0
ClrLoop:
    stmia   r0!, {r2}
    subs    r1, r1, #4
    bne     ClrLoop
    bx      lr

.section .__bss_start
.global __c_bss_start
__c_bss_start:

.section .__bss_end
.global __c_bss_end
__c_bss_end:
