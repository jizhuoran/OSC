//
//  memlayout.h
//  ucore-myself
//
//  Created by jizhuoran on 16/7/5.
//  Copyright © 2016年 jizhuoran. All rights reserved.
//

#ifndef memlayout_h
#define memlayout_h


/* *
 * Virtual memory map:                                          Permissions
 *                                                              kernel/user
 *
 *     4G ------------------> +---------------------------------+
 *                            |                                 |
 *                            |         Empty Memory (*)        |
 *                            |                                 |
 *                            +---------------------------------+ 0xFB000000
 *                            |   Cur. Page Table (Kern, RW)    | RW/-- PTSIZE
 *     VPT -----------------> +---------------------------------+ 0xFAC00000
 *                            |        Invalid Memory (*)       | --/--
 *     KERNTOP -------------> +---------------------------------+ 0xF8000000
 *                            |                                 |
 *                            |    Remapped Physical Memory     | RW/-- KMEMSIZE
 *                            |                                 |
 *     KERNBASE ------------> +---------------------------------+ 0xC0000000
 *                            |                                 |
 *                            |                                 |
 *                            |                                 |
 *                            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * (*) Note: The kernel ensures that "Invalid Memory" is *never* mapped.
 *     "Empty Memory" is normally unmapped, but user programs may map pages
 *     there if desired.
 *
 * */

/* All physical memory mapped at this address */
#define KERNBASE            0xC0000000 //kernel 的起始地址
#define KMEMSIZE            0x38000000 //kernel 的大小
#define KERNTOP             (KERNBASE + KMEMSIZE) //kernel 内存空间的顶部
#define VPT                 0xFAC00000 //储存此页表的位置



#ifndef __ASSEMBLER__

#include "list.h"
typedef uintptr_t pte_t;
typedef uintptr_t pde_t;


#define E820MAX             20      // number of entries in E820MAP
#define E820_ARM            1       // address range memory
#define E820_ARR            2       // address range reserved

struct e820map {
    int nr_map;
    struct {
        uint64_t addr;
        uint64_t size;
        uint32_t type;
    } __attribute__((packed)) map[E820MAX];
};

struct Page {
    int ref;                        // //这个页被引用的次数
    uint32_t flags;                 // 这个页的flags
    unsigned int property;          // 空闲的块，在first fit分配算法中用
    list_entry_t page_link;         // free list link
};



#define PG_reserved                 0       // the page descriptor is reserved for kernel or unusable
#define PG_property                 1       // the member 'property' is valid

#define SetPageReserved(page)       set_bit(PG_reserved, &((page)->flags))
#define ClearPageReserved(page)     clear_bit(PG_reserved, &((page)->flags))
#define PageReserved(page)          test_bit(PG_reserved, &((page)->flags))
#define SetPageProperty(page)       set_bit(PG_property, &((page)->flags))
#define ClearPageProperty(page)     clear_bit(PG_property, &((page)->flags))
#define PageProperty(page)          test_bit(PG_property, &((page)->flags))




typedef struct {
    list_entry_t free_list;         // 链表的头
    unsigned int nr_free;           // free list 的数量
} free_area_t;

#endif //ASSEMEBLER

#endif /* memlayout_h */
