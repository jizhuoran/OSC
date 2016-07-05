#include <asm.h>

.set PROT_MODE_CSEG,        0x8                     # kernel code segment selector
.set PROT_MODE_DSEG,        0x10                    # kernel data segment selector
.set CR0_PE_ON,             0x1                     # protected mode enable flag
.set SMAP,                  0x534d4150

.globl start
start:
.code16
    cli #禁止中断(Disable interrupts)
    cld #使得传送方向从低地址到高地址(String operations increment)

    worw %ax, %ax #%ax = 0
    movw %ax, %de #%de = 0
    movw %ax, %es #%es = 0
    movw %ax, %ss #%ss = 0


#开启A20(Enable A20)

/*

在PC刚出现的时候，CPU只有一款，那就是 8086 ,因为它和后续的 8088 既便宜又耐操，所以很快流行起来。这颗CPU有16位的寄存器，但是却有20条 地址线 [2] ，所以 Intel 发明了臭名昭著的用段寄存器访问更多内存的方 法。

举个例子， abcd:1234 这个地址（16进制），冒号前面的是段寄存器的值， 后面的是程序中访问的地址，那么真正的物理地址计算方法是 0xabcd * 0x10 + 0x1234 = 0xacf04 . 这是个20位的地址，刚好可以用在 8086的地址总线上。

这个计算方式有个很微妙的问题： ffff:ffff 这个最大的地址映射到物理地 址 0x10ffef ，TMD都超过20位了…… Intel 的解决方法是装作没看见第21位， 将这个地址当作 0xffef 去访问……

所以，当时的程序是可以通过访问 1MB 以上的地址，来获得物理地址 0xffef 之前的数据的；也真有程序利用了这一点，从而省掉载入段寄存器的操作。

接下来 Intel 与时俱进推出了80286，它还是16位的CPU，但是地址总线一下子 扩展到24位，所以CPU不能再对第21位地址视而不见了。当新的程序访问 ffff:ffff 这个地址时，它有可能是真的想访问物理地址 0x10ffef ；但是当 旧的程序访问 ffff:ffff 时，它肯定是想要访问 0xffef .

由于兼容旧程序是抢占市场的重要手段 [3] ， Intel 决定让80286默认以 8086一样的行为工作，也就是对第21位地址视而不见，总是将 A20 置为零。 当程序确定它想要访问 1MB 以上的内存时，再通过特定的方式打开 A20.

而这个特定的方式——不知道当时 Intel 那帮人怎么想的——就是用键盘控制器 上多出来的一个状态位。据说原因就是，有人发现那一位刚.好.多.出.来.了……

于是就出现了 boot loader 里捣鼓 0x64 和 0x60 这两个 port 的代码。

*/

seta20.1:
    inb $0x64, %al #读取状态(get state)
    testb $0x2, %al #忙？(busy?)
    jnz seta20.1 #为真，跳转(yes)

    movb $0xd1, %al #(al=0xd1)
    outb %al, $0x64 #写状态到0x64(write the state)

seat20.2:
    inb $0x64, %al #读取状态(get state)
    testb $0x2, %al #忙？(busy?)
    jnz seta20.2 #为真，跳转(yes)

    movb $0xdf, %al #(al=0xdf)
    outb %al, $0x60 #写状态到0x60(write the state)

#侦测物理内存(probe the physical memory)
/*
操作系统需要知道了解整个计算机系统中的物理内存如何分布的，哪些被可用，哪些不可用。其基本方法是通过BIOS中断调用来帮助完成的。其中BIOS中断调用必须在实模式下进行，所以在bootloader进入保护模式前完成这部分工作相对比较合适。这些部分由boot/bootasm.S中从probe_memory处到finish_probe处的代码部分完成完成。通过BIOS中断获取内存可调用参数为e820h的INT 15h BIOS中断。BIOS通过系统内存映射地址描述符（Address Range Descriptor）格式来表示系统物理内存布局，其具体表示如下：

        Offset  Size    Description
        00h    8字节   base address               #系统内存块基地址
        08h    8字节   length in bytes            #系统内存大小
        10h    4字节   type of address range     #内存类型
看下面的(Values for System Memory Map address type)

Values for System Memory Map address type:
        01h    memory, available to OS
        02h    reserved, not available (e.g. system ROM, memory-mapped device)
        03h    ACPI Reclaim Memory (usable by OS after reading ACPI tables)
        04h    ACPI NVS Memory (OS is required to save this memory between NVS sessions)
        other  not defined yet -- treat as Reserved
INT15h BIOS中断的详细调用参数:

        eax：e820h：INT 15的中断调用参数；
        edx：534D4150h (即4个ASCII字符“SMAP”) ，这只是一个签名而已；
        ebx：如果是第一次调用或内存区域扫描完毕，则为0。 如果不是，则存放上次调用之后的计数值；
        ecx：保存地址范围描述符的内存大小,应该大于等于20字节；
        es:di：指向保存地址范围描述符结构的缓冲区，BIOS把信息写入这个结构的起始地址。
此中断的返回值为:

    cflags的CF位：若INT 15中断执行成功，则不置位，否则置位；

        eax：534D4150h ('SMAP') ；

        es:di：指向保存地址范围描述符的缓冲区,此时缓冲区内的数据已由BIOS填写完毕

        ebx：下一个地址范围描述符的计数地址

        ecx    ：返回BIOS往ES:DI处写的地址范围描述符的字节大小

        ah：失败时保存出错代码
这样，我们通过调用INT 15h BIOS中断，递增di的值（20的倍数），让BIOS帮我们查找出一个一个的内存布局entry，并放入到一个保存地址范围描述符结构的缓冲区中，供后续的ucore进一步进行物理内存管理。这个缓冲区结构定义在memlayout.h中：

        struct e820map {
                        int nr_map;
                        struct {
                                            long long addr;
                                            long long size;
                                            long type;
                        } map[E820MAX];
        };


*/


probe_memory:
    movl $0, 0x8000 #对0x8000处的32位单元清零,即给位于0x8000处的struct e820map的成员变量nr_map清零
    xorl %ebx, %ebx # %ebx = 0
    movw $0x8004, %di #表示设置调用INT 15h BIOS中断后，BIOS返回的映射地址描述符的起始地址

start_probe:
    movl $0xE820, %eax #INT 15的中断调用参数
    movl $20, $ecx #设置地址范围描述符的大小为20字节，其大小等于struct e820map的成员变量map的大小
    movl $SMAP, %edx #设置edx为534D4150h (即4个ASCII字符“SMAP”)，这是一个约定
    int $0x15 #调用int 0x15中断，要求BIOS返回一个用地址范围描述符表示的内存段信息
    jnc cont #如果eflags的CF位为0，则表示还有内存段需要探测
    movw $12345, 0x8000 #表示侦测出现问题，不然应该在cont跳过执行jnz而到达finish_probe
    jmp finish_probe

cont:
    addw $20, %di #设置下一个BIOS返回的映射地址描述符的起始地址
    incl 0x8000 #递增struct e820map的成员变量nr_map
    cmpl $0, %ebx #如果INT0x15返回的ebx为零，表示探测结束，否则继续探测
    jnz start_probe

finish_probe:

    lgdt gdtdesc #将GDT的基地址和大小传入到寄存器gdtr中
    movl %cr0, %eax #将cr0里的数据加载到％eax里
    orl CR0_PE_ON, %eax #CR0_PE_ON的数据是0x1(0001)，和cr0做或运算，相当于把cr0的坠后一位设置成1
    movl %eax, cr0 #将设置好的数据传回到cr0里

    ljmp $PROT_MODE_CSEG, $protcseg #将PROT_MODE_CSEG加载到cs里，并把pc指向protcseg

.code32                                             # Assemble for 32-bit mode
protcseg:
    # 设置保护模式（Set up the protected-mode data segment registers）
    movw $PROT_MODE_DSEG, %ax                       # Our data segment selector
    movw %ax, %ds                                   # -> DS: Data Segment
    movw %ax, %es                                   # -> ES: Extra Segment
    movw %ax, %fs                                   # -> FS
    movw %ax, %gs                                   # -> GS
    movw %ax, %ss                                   # -> SS: Stack Segment

    movl $0x0, %ebp #把ebp设置成0
    movl $start, %esp #把栈指针指向start（0x7c00）
    call bootmain #调用C函数bootmain

    # 如果bootmain返回了，就spin处理器
spin:
    jmp spin

.data
# Bootstrap GDT
.p2align 2                                          # force 4 byte alignment
gdt:
    SEG_NULLASM                                     # null seg
    SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)           # code seg for bootloader and kernel
    SEG_ASM(STA_W, 0x0, 0xffffffff)                 # data seg for bootloader and kernel

gdtdesc:
    .word 0x17                                      # GDTR界限，可以描述(0x27+1)/8=5个描述符
    .long gdt                                       # GDTR基地址，指向gdt








