/* mini_jmp.S - setjmp/longjmp for the freestanding build (Lua's error handling).
 * Saves/restores the callee-saved registers, stack pointer and return address. */
.text
.global _setjmp
.global setjmp
.global longjmp
_setjmp:
setjmp:
    movq %rbx,  0(%rdi)
    movq %rbp,  8(%rdi)
    movq %r12, 16(%rdi)
    movq %r13, 24(%rdi)
    movq %r14, 32(%rdi)
    movq %r15, 40(%rdi)
    leaq 8(%rsp), %rax
    movq %rax, 48(%rdi)
    movq (%rsp), %rax
    movq %rax, 56(%rdi)
    xorl %eax, %eax
    ret
longjmp:
    movq  0(%rdi), %rbx
    movq  8(%rdi), %rbp
    movq 16(%rdi), %r12
    movq 24(%rdi), %r13
    movq 32(%rdi), %r14
    movq 40(%rdi), %r15
    movq 48(%rdi), %rsp
    movl %esi, %eax
    testl %eax, %eax
    jnz 1f
    movl $1, %eax
1:  jmpq *56(%rdi)
