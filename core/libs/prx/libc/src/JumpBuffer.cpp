#include <cstdint>

#include "prx/libc/include/General.hpp"

// Guest code is SysV x86-64 and owns its opaque jmp_buf, so setjmp/longjmp are implemented directly
// in assembly over the guest buffer and never touch the MinGW host setjmp (Microsoft layout, ~256
// bytes, unwind-aware). Buffer layout, 8 qwords, one buffer per set point:
//   0 rbx   8 rbp   16 r12   24 r13   32 r14   40 r15   48 rsp (after return)   56 rip (return address)
// Only the SysV callee-saved registers are preserved, which is all guest code relies on.

extern "C" {

__attribute__((naked, noinline)) int APS5_VABI setjmp_nid_postfix(void* env) {
    asm volatile(
        "movq  (%rsp), %rax\n\t"
        "movq  %rax, 56(%rdi)\n\t"
        "leaq  8(%rsp), %rax\n\t"
        "movq  %rax, 48(%rdi)\n\t"
        "movq  %rbx, 0(%rdi)\n\t"
        "movq  %rbp, 8(%rdi)\n\t"
        "movq  %r12, 16(%rdi)\n\t"
        "movq  %r13, 24(%rdi)\n\t"
        "movq  %r14, 32(%rdi)\n\t"
        "movq  %r15, 40(%rdi)\n\t"
        "xorl  %eax, %eax\n\t"
        "ret");
}

[[noreturn]] __attribute__((naked, noinline)) void APS5_VABI longjmp_nid_postfix(void* env, int val) {
    asm volatile(
        "movl  %esi, %eax\n\t"
        "testl %eax, %eax\n\t"
        "jnz   1f\n\t"
        "movl  $1, %eax\n\t"
        "1:\n\t"
        "movq  0(%rdi), %rbx\n\t"
        "movq  8(%rdi), %rbp\n\t"
        "movq  16(%rdi), %r12\n\t"
        "movq  24(%rdi), %r13\n\t"
        "movq  32(%rdi), %r14\n\t"
        "movq  40(%rdi), %r15\n\t"
        "movq  48(%rdi), %rsp\n\t"
        "jmpq  *56(%rdi)");
}

}
