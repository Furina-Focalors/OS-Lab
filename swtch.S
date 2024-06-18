# Context switch
#
#   void swtch(struct context **old, struct context *new);
# 
# Save the current registers on the stack, creating
# a struct context, and save its address in *old.
# Switch stacks to new and pop previously-saved registers.
# %eax: 累加器寄存器，也用作通用寄存器，很多指令中作为默认的目标寄存器，也常用于存储函数的返回值
# %edx: 数据寄存器，也用作通用寄存器
# %esp: 堆栈指针寄存器，包含一个指向当前栈顶的指针
# (%regname)表示间接寻址，即寄存器中的内容为一个地址

.globl swtch
swtch:
  movl 4(%esp), %eax # 加载第一个参数**old至eax寄存器，这个参数是一个指向原来进程的内核栈的指针
  movl 8(%esp), %edx # 加载第二个参数*new至edx寄存器，这个参数是一个指向新进程的内核栈的指针

  # Save old callee-saved registers
  pushl %ebp
  pushl %ebx
  pushl %esi
  pushl %edi

  # Switch stacks
  movl %esp, (%eax)
  movl %edx, %esp

  # Load new callee-saved registers
  popl %edi
  popl %esi
  popl %ebx
  popl %ebp
  ret
