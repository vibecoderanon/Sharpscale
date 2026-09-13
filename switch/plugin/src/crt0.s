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
    // Returns 0 safely to SaltySD loader
    mov x0, #0
    ret
