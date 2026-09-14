.section .crt0, "ax", %progbits
.global _start
.align 2

_start:
    b entrypoint
    .word __nx_mod0 - _start

.org _start+0x20
.global __nx_mod0
__nx_mod0:
    .ascii "MOD0"
    .word  _DYNAMIC             - __nx_mod0
    .word  __bss_start__        - __nx_mod0
    .word  __bss_end__          - __nx_mod0
    .word  0
    .word  0
    .word  0

.org _start+0x80
entrypoint:
    // SaltyNX plugin entrypoint called via elf_trampoline
    // Branch with link to sharpscale_init
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    bl sharpscale_init
    ldp x29, x30, [sp], #16
    mov x0, #0
    ret

.global raw_svcGetInfo
.type raw_svcGetInfo, %function
raw_svcGetInfo:
    str x0, [sp, #-16]!
    svc 0x29
    ldr x2, [sp], #16
    cbz w0, 1f
    ret
1:
    cbz x2, 2f
    str x1, [x2]
2:
    ret

.global raw_svcQueryMemory
.type raw_svcQueryMemory, %function
raw_svcQueryMemory:
    str x1, [sp, #-16]!
    svc 0x6
    ldr x2, [sp], #16
    cbz w0, 1f
    ret
1:
    cbz x2, 2f
    str w1, [x2]
2:
    ret

.global raw_svcMapSharedMemory
.type raw_svcMapSharedMemory, %function
raw_svcMapSharedMemory:
    svc 0x13
    ret

.global raw_svcUnmapSharedMemory
.type raw_svcUnmapSharedMemory, %function
raw_svcUnmapSharedMemory:
    svc 0x14
    ret


