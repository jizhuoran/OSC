#include "elf.h"
#include "defs.h"
#include "x86.h"

#define SECTSIZE 512
#define ELFHDR ((struct elfhdr *)0x10000)
/*
 此处介绍ELF header
 e_type 它标识的是该文件的类型。
 e_machine 表明运行该程序需要的体系结构。
 e_version 表示文件的版本。
 e_entry 程序的入口地址。
 e_phoff 表示Program header table 在文件中的偏移量（以字节计数）。
 e_shoff 表示Section header table 在文件中的偏移量（以字节计数）。
 e_flags 对IA32而言，此项为0。
 e_ehsize 表示ELF header大小（以字节计数）。
 e_phentsize 表示Program header table中每一个条目的大小。
 e_phnum 表示Program header table中有多少个条目。
 e_shentsize 表示Section header table中的每一个条目的大小。
 e_shnum 表示Section header table中有多少个条目。
 e_shstrndx 包含节名称的字符串是第几个节（从零开始计数）。
*/

static void
readsect(void *dst, uint32_t secno) {
    waitdisk(); //等待硬盘
    
    outb(0x1F2, 1);                         // count = 1
    outb(0x1F3, secno & 0xFF);
    outb(0x1F4, (secno >> 8) & 0xFF);
    outb(0x1F5, (secno >> 16) & 0xFF);
    outb(0x1F6, ((secno >> 24) & 0xF) | 0xE0);
    outb(0x1F7, 0x20);                      // cmd 0x20 - read sectors
    
    waitdisk(); //等待硬盘
    
    insl(0x1F0, dst, SECTSIZE / 4);//读取这个sector
}

static void readseg(uintptr_t va, uint32_t count, uint32_t offset) {
    uintptr_t end_va = va + count; //计算结尾
    va -= offset % SECTSIZE; //因为需要一个sector一个sector的传输，所以需要把va设置成原来的va所在的sector的头
    uint32_t secno = (offset / SECTSIZE) + 1;//找到要读的第一个sector
    for (; va < end_va; va += SECTSIZE, ++secno) { //用va坐循环变量，但是读的是sector
        readsect((void *)va, secno);
    }
}


void bootmain(void) {
    readseg((uintptr_t)ELFHDR, SECTSIZE * 8, 0);//读取ELF header
    if(ELFHDR->e_magic != ELF_MAGIC) { //如果e_magic不是ELF_MAGIC，进入bad模式
        goto bad;
    }
    
    struct proghdr *ph, *eph;//建立prog header，ph代表头，eph代表尾
    /*
     uint32_t p_type;   // 它标识的是该program的类型
     uint32_t p_offset; // 这个segment在文件中的offset
     uint32_t p_va;     // 这个segment对应的virtual memory
     uint32_t p_pa;     // physical address, not used
     uint32_t p_filesz; // 这个segment在文件中的大小
     uint32_t p_memsz;  // 这个segment在内存中的大小
     uint32_t p_flags;  // read/write/execute bits
     uint32_t p_align;  // required alignment, invariably hardware page size
    */
    ph = (struct proghdr *)((uintptr_t)ELFHDR + ELFHDR->e_phoff); //计算ph
    eph = ph + ELFHDR->e_phnum; //计算eph
    for(; ph < eph; ++ph) { //对于每一个segment
        readseg(ph->p_va & 0xFFFFFF, ph->p_memsz, ph->p_offset);//读取这个segment
    }
    ((void (*)(void))(ELFHDR->e_entry & 0xFFFFFF))();//调用这个程序(kern/init.c中的kern_init)
    
bad:
    outw(0x8A00, 0x8A00);
    outw(0x8A00, 0x8E00);
    
    while(1);
    
}
