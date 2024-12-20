[bits 32]
[extern main] ; Declare that we will be referencing the external symbol 'main',
             ; so the linker can substitute the final address

call main ; Invoke main() in our C kernel
jmp $ ; Hang forever when we're done

%include "print_string.asm"
