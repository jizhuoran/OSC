#include "stdio.h"
#include "x86.h"
#include "console.h"
#include "pmm.h"
#include "trap.h"
#include "clock.h"
#include "picirq.h"
#include "intr.h"
#include "string.h"




int kern_init(void) {
    extern char edata[], end[];
    memset(edata, 0, end-edata);
    
    cons_init(); //初始化console
    
    const char *message = "OS is loading ...";
    cprintf("%s\n\n", message);
    
    pmm_init(); //初始化物理内存管理
    
    pic_init();                 // 初始化中断控制器
    idt_init();                 // 初始化 interrupt descriptor table
    
    clock_init();               // 初始化 clock interrupt
    intr_enable();              // enable irq interrupt
    
    while(1);
}

