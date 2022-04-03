
;
; From: https://stackoverflow.com/questions/8453146/128-bit-division-intrinsic-in-visual-c
;
; extern "C" uint64_t __udiv128(uint64_t low, uint64_t high, uint64_t divisor, uint64_t * remainder);
;
; Arguments
; RCX       Low Digit
; RDX       High Digit
; R8        Divisor
; R9        *Remainder

; RAX       Quotient upon return

.code
__udiv128 proc
    mov rax, rcx    ; Put the low digit in place (hi is already there)
    div r8          ; 128 bit divide rdx-rax/r8 = rdx remainder, rax quotient
    mov [r9], rdx   ; Save the reminder
    ret             ; Return the quotient
__udiv128 endp
end

;
; NASM: C:\usr\bin\NASM\nasm.exe -f win32 -o $(OutputPath)__udiv128.obj $(SolutionDir)src\jstd\udiv128.asm
;
; https://www.cs.uaf.edu/2009/fall/cs301/nasm_vcpp/
;