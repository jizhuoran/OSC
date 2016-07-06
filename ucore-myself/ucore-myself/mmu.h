//
//  mmu.h
//  ucore-myself
//
//  Created by jizhuoran on 16/7/5.
//  Copyright © 2016年 jizhuoran. All rights reserved.
//

#ifndef mmu_h
#define mmu_h

#include "defs.h"


// A linear address 'la' has a three-part structure as follows:
//
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |     Index      |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(la) --/ \--- PTX(la) --/ \---- PGOFF(la) ----/
//  \----------- PPN(la) -----------/
//
// The PDX, PTX, PGOFF, and PPN macros decompose linear addresses as shown.
// To construct a linear address la from PDX(la), PTX(la), and PGOFF(la),
// use PGADDR(PDX(la), PTX(la), PGOFF(la)).


#define PDX(la) ((((uintptr_t)(la)) >> PDXSHIFT) & 0x3FF) //获得10位的page directory
#define PTX(la) ((((uintptr_t)(la)) >> PTXSHIFT) & 0x3FF) //获得10位的page table
#define PPN(la) (((uintptr_t)(la)) >> PTXSHIFT) //20位，page directory和page table的和
#define PGOFF(la) (((uintptr_t)(la)) & 0xFFF) //偏移值


#define NPTEENTRY 1024 //一个page table 的 entries
#define NPDEENTRY 1024 //一个page directory 的 entries

#define PGSIZE 4096 //一个页所对应的bytes
#define PGSHIFT 12 //一个页的位数
#define PTSHIFT 22 //一个page table的位数
#define PTSIZE (PGSIZE * NPTEENTRY) //一个page table 所对应的bytes



#define PTXSHIFT        12 //取la的前20位
#define PDXSHIFT        22 //取la的前10位


/* page table/directory entry flags */
#define PTE_P           0x001                   // Present
#define PTE_W           0x002                   // Writeable
#define PTE_U           0x004                   // User
#define PTE_PWT         0x008                   // Write-Through
#define PTE_PCD         0x010                   // Cache-Disable
#define PTE_A           0x020                   // Accessed
#define PTE_D           0x040                   // Dirty
#define PTE_PS          0x080                   // Page Size
#define PTE_MBZ         0x180                   // Bits must be zero
#define PTE_AVAIL       0xE00                   // Available for software use
// The PTE_AVAIL bits aren't used by the kernel or interpreted by the
// hardware, so user processes are allowed to set them arbitrarily.




#endif /* mmu_h */
