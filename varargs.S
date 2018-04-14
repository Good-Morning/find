.intel_syntax noprefix

.section .text
.balign 16

.globl get_var_arg
get_var_arg:
lea rax, [rbp + 16]
ret

.globl get_num_arg
get_num_arg:
mov rax, [rbp + 8]
cmp word ptr[rax], 50307
jne _get_num_arg_zero
movzx rax, byte ptr[rax + 2]
ret
_get_num_arg_zero:
xor rax, rax
ret