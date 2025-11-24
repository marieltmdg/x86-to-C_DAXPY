; BAYBAYON & TAMONDONG S20A
section .text
bits 64
default rel

global daxpy_asm
extern malloc

; rcx = n
; xmm1 = A
; r8  = X pointer
; r9  = Y pointer
; Output: rax = pointer to Z (or NULL if malloc fails)
; computes Z[i] = A*X[i] + Y[i]  (double precision)

daxpy_asm:
    push rbp
    mov  rbp, rsp
    push r12
    push r13
    push r14
    sub  rsp, 40            

    mov  r12, rcx           ; n
    mov  r13, r8            ; X pointer in non-volatile
    mov  r14, r9            ; Y pointer in non-volatile
    movsd xmm15, xmm1       ; A in non-volatile

    lea  rcx, [r12*8]       ; size = n * 8
    call malloc
    test rax, rax
    jz   .malloc_fail
    mov  r10, rax           ; r10 = base Z pointer (keep base)
    mov  r11, r10           ; r11 = current Z pointer

.loop:
    test r12, r12
    jz   .done

    movsd xmm0, [r13]       ; load X
    mulsd xmm0, xmm15       ; xmm0 = A * X
    addsd xmm0, [r14]       ; xmm0 += Y
    movsd [r11], xmm0       ; store into Z

    add  r13, 8             ; X++
    add  r14, 8             ; Y++
    add  r11, 8             ; Z_current++
    dec  r12
    jmp  .loop

.done:
    mov  rax, r10           ; return Z pointer base 

    add  rsp, 40
    pop  r14
    pop  r13
    pop  r12
    pop  rbp
    ret

.malloc_fail:
    xor  rax, rax
    add  rsp, 40
    pop  r14
    pop  r13
    pop  r12
    pop  rbp
    ret
