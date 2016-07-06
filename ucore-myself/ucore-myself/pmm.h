
#ifndef pmm_h
#define pmm_h

#include "defs.h"
#include "assert.h"
#include "memlayout.h"
#include "mmu.h"


struct pmm_manager {
    const char *name;                                 // XXX_pmm_manager的名字
    void (*init)(void);
    // initialize internal description&management data structure
    // (free block list, number of free block) of XXX_pmm_manager
    void (*init_memmap)(struct Page *base, size_t n); // setup description&management data structcure according to
    // the initial free physical memory space
    struct Page *(*alloc_pages)(size_t n);            // allocate >=n pages, depend on the allocation algorithm
    void (*free_pages)(struct Page *base, size_t n);  // free >=n pages with "base" addr of Page descriptor structures(memlayout.h)
    size_t (*nr_free_pages)(void);                    // return the number of free pages
    void (*check)(void);                              // check the correctness of XXX_pmm_manager
};


void pmm_init (void);



/* *
 取一个kernel的virtual address，返回对应的物理地址
 * PADDR - takes a kernel virtual address (an address that points above KERNBASE),
 * where the machine's maximum 256MB of physical memory is mapped and returns the
 * corresponding physical address.  It panics if you pass it a non-kernel virtual address.
 * */
#define PADDR(kva) ({                                                   \
uintptr_t __m_kva = (uintptr_t)(kva);                       \
if (__m_kva < KERNBASE) {                                   \
panic("PADDR called with invalid kva %08lx", __m_kva);  \
}                                                           \
__m_kva - KERNBASE;                                         \
})

/* *
 取一个物理地址，返回对应的kernel virtual address
 * KADDR - takes a physical address and returns the corresponding kernel virtual
 * address. It panics if you pass an invalid physical address.
 * */
#define KADDR(pa) ({                                                    \
uintptr_t __m_pa = (pa);                                    \
size_t __m_ppn = PPN(__m_pa);                               \
if (__m_ppn >= npage) {                                     \
panic("KADDR called with invalid pa %08lx", __m_pa);    \
}                                                           \
(void *) (__m_pa + KERNBASE);                               \
})

extern struct Page *pages;
extern size_t npage;

static inline ppn_t page2ppn(struct Page *page) {
    return page - pages;   //这个函数返回page在页表里的index
}

static inline uintptr_t page2pa(struct Page *page) {
    return page2ppn(page) << PGSHIFT;
    //将上面返回的index左移12位，把页表index放到正确的位置上
    //操作之后相当于找到了这个页index所指向的页的第一个位置
}


static inline void *page2kva(struct Page *page) {
    return KADDR(page2pa(page)); //根据上面返回的位置，找到对应的kernel virtual address
}


#endif /* pmm_h */
