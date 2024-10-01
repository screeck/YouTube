global _main            ; Declare the _main function globally so that the linker can find it.

section .text           ; Begin the 'text' section where the code resides.

_main:                  ; Label for the main function entry point.
    mov ecx, len        ; Move the length of the array (6) into register ECX, which will control the outer loop.
    dec ecx             ; Decrement ECX by 1, as the array is 0-indexed and sorting compares elements, not the length.

outer_loop:             ; Label for the start of the outer loop, which performs multiple passes over the array.
    mov esi, 0          ; Initialize ESI to 0, this is the index pointer used to traverse the array.
    mov edx, ecx        ; Copy the value of ECX (remaining length) to EDX for use in the inner loop.

inner_loop:             ; Label for the start of the inner loop, which performs pairwise comparisons.
    mov eax, [array + esi*4]      ; Load the value of the array at index ESI (first number of the pair) into EAX.
    mov ebx, [array + esi*4 + 4]  ; Load the value of the array at index ESI+1 (second number of the pair) into EBX.
    cmp eax, ebx                  ; Compare EAX and EBX to see if the pair is in the correct order.
    jle no_swap                   ; Jump to 'no_swap' if EAX <= EBX (no need to swap the values).

    ; Swap the two values if EAX > EBX
    mov [array + esi*4], ebx      ; Move the value in EBX (larger value) into the first array position (ESI).
    mov [array + esi*4 + 4], eax  ; Move the value in EAX (smaller value) into the second array position (ESI+1).

no_swap:               ; Label for when no swap is needed, continue to the next pair.
    inc esi            ; Increment ESI to point to the next pair of elements in the array.
    dec edx            ; Decrement EDX, which counts how many comparisons are left in the inner loop.
    jnz inner_loop     ; If EDX is not zero, repeat the inner loop for the next pair.

    dec ecx            ; Decrement ECX to reduce the range for the next pass (outer loop shrinks the range).
    jnz outer_loop     ; If ECX is not zero, repeat the outer loop for another pass over the array.

    ; Exit the program
    mov eax, 60        ; System call number 60 (exit) for the Linux syscall convention.
    xor edi, edi       ; Set EDI to 0, which means the program exits with status code 0 (successful exit).
    syscall            ; Trigger the system call to exit the program.

section .data          ; Begin the 'data' section where data variables are defined.

array: dd 5,3,4,2,6,1  ; Define an array of 6 integers: 5, 3, 4, 2, 6, 1 (unsorted).
len equ 6              ; Define a constant 'len' which is the length of the array (6 elements).
