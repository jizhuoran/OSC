#include "pmm.h"
#include "mmu.h"
#include "memlayout.h"
#include "default_pmm.h"
#include "stdio.h"
#include "defs.h"
#include "string.h"
#include "assert.h"


const struct pmm_manager *pmm_manager;
struct Page *pages;


static void check_pgdir(void);
static void check_alloc_page(void);

pde_t *boot_pgdir = NULL; //boot时候的page directory的虚拟地址
uintptr_t boot_cr3; //boot时候的page directory的物理地址
size_t npage = 0;


static void
boot_map_segment(pde_t *pgdir, uintptr_t la, size_t size, uintptr_t pa, uint32_t perm) {
    //  la:   需要map的线性地址
    //  size: 内存大小
    //  pa:   物理地址
    //  perm: 许可权
    assert(PGOFF(la) == PGOFF(pa));//检测偏移值是否相同，一般来说就是看是不是从一个页开始
    size_t n = ROUNDUP(size + PGOFF(la), PGSIZE) / PGSIZE;
    //从偏移量开始数size个内存，然后向上取整，除以PGSIZE看有多少页
    la = ROUNDDOWN(la, PGSIZE);
    pa = ROUNDDOWN(pa, PGSIZE);
    //因为内存需要按页分配，将起始位置向下取整来分配
    for (; n > 0; n --, la += PGSIZE, pa += PGSIZE) { //对于每一个需要分配的页
        pte_t *ptep = get_pte(pgdir, la, 1); //获得pte的虚拟地址
        assert(ptep != NULL);
        *ptep = pa | PTE_P | perm; //？？？？？？？？？
    }
}



static void init_pmm_manager(void) {
    pmm_manager = &default_pmm_manager; //选定default_pmm_manager为pmm_manager
    cprintf("memory management: %s\n", pmm_manager->name);
    pmm_manager->init();//调用这个pmm_manager的init函数
}

static void page_init(void) {
    struct e820map *memmap = (struct e820map *)(0x8000 + KERNBASE);//之前在bootasm.S里侦测到内存
    uint64_t maxpa;
    cprintf("e820map:\n");
    int i;
    for (i = 0; i < memmap->nr_map; i ++) { //对于每一个entries
        uint64_t begin = memmap->map[i].addr, end = begin + memmap->map[i].size;
        //找到他的起始和结束
        cprintf("  memory: %08llx, [%08llx, %08llx], type = %d.\n",
                memmap->map[i].size, begin, end - 1, memmap->map[i].type);
        if (memmap->map[i].type == E820_ARM) { //如果他没有被占用
            if (maxpa < end && begin < KMEMSIZE) { //因为kernel只能用这么多memory
                maxpa = end;
            }
        }
    }
    
    if (maxpa > KMEMSIZE) {
        maxpa = KMEMSIZE;
    }
    
    extern char end[];
    
    npage = maxpa / PGSIZE; //看内核可用的内存地址有多少页
    pages = (struct Page *)ROUNDUP((void *)end, PGSIZE);
    //此处的end不是循环里的end，因为在循环块的外面，这里的end是指bootloader加载ucore的结束地址,所以pages是页表的第一项
    
    for (i = 0; i < npage; i ++) {
        SetPageReserved(pages + i);
    }
    
    uintptr_t freemem = PADDR((uintptr_t)pages + sizeof(struct Page) * npage);
    //第一个可以用的块就是ucore和页表之后的位置
    
    for (i = 0; i < memmap->nr_map; i ++) {
        uint64_t begin = memmap->map[i].addr, end = begin + memmap->map[i].size;
        if (memmap->map[i].type == E820_ARM) {
            if (begin < freemem) {
                begin = freemem;
            }   //如果块的起始比freemem还小，就设置他为freemem
            if (end > KMEMSIZE) {
                end = KMEMSIZE;
            } //如果块的结束比KMEMSIZE还大，就设置他为KMEMSIZE
            if (begin < end) {  //如果这个块是合法的块
                //就把它们round到page对齐
                begin = ROUNDUP(begin, PGSIZE);
                end = ROUNDDOWN(end, PGSIZE);
                //round完可能是空块，所以要检查一下
                if (begin < end) {
                    //第一个参量是这个地址对应的page，init_memmap则讲Page结构的flags和引用计数清零，并把他加入到free_area.free_list里
                    init_memmap(pa2page(begin), (end - begin) / PGSIZE);
                }
            }
        }
    }
}


static void *boot_alloc_page(void) {
    struct Page *p = alloc_page(); //分配一个页
    if( p == NULL) {
        panic("boot_alloc_page failed.\n");
    }
    return page2kva(p); //返回这个页的虚拟地址
}

void pmm_init (void) {
    init_pmm_manager(); //初始化物理内存管理器
    page_init(); //这个函数为kernel初始化页
    check_alloc_page(); //检查页是否分配成功
    
    boot_pgdir = boot_alloc_page(); //给boot时候的page directory分配一个页，指向第一个位置
    memset(boot_pgdir, 0, PGSIZE); //把这个页全部初始化成0
    boot_cr3 = PADDR(boot_pgdir); //指向boot时候的page directory的物理内存的第一个位置
    
    check_pgdir();
    
    static_assert(KERNBASE % PTSIZE == 0 && KERNTOP % PTSIZE == 0);
    
    boot_pgdir[PDX(VPT)] = PADDR(boot_pgdir) | PTE_P | PTE_W;
    //把这个页表的物理地址储存在它本身的PDX（VPT）上
    
    boot_map_segment(boot_pgdir, KERNBASE, KMEMSIZE, 0, PTE_W);//将线性内存和物理内存做映射
    boot_pgdir[0] = boot_pgdir[PDX(KERNBASE)];//
    
    enable_paging(); //开启paging模式
    
    gdt_init();
    
    boot_pgdir[0] = 0;
    
    check_boot_pgdir();
    
    print_pgdir();
    
}

static void check_alloc_page(void) {
    pmm_manager->check();
    cprintf("check_alloc_page() succeeded!\n");
}

static void
check_pgdir(void) {
    assert(npage <= KMEMSIZE / PGSIZE);
    assert(boot_pgdir != NULL && (uint32_t)PGOFF(boot_pgdir) == 0);
    assert(get_page(boot_pgdir, 0x0, NULL) == NULL);
    
    struct Page *p1, *p2;
    p1 = alloc_page();
    assert(page_insert(boot_pgdir, p1, 0x0, 0) == 0);
    
    pte_t *ptep;
    assert((ptep = get_pte(boot_pgdir, 0x0, 0)) != NULL);
    assert(pte2page(*ptep) == p1);
    assert(page_ref(p1) == 1);
    
    ptep = &((pte_t *)KADDR(PDE_ADDR(boot_pgdir[0])))[1];
    assert(get_pte(boot_pgdir, PGSIZE, 0) == ptep);
    
    p2 = alloc_page();
    assert(page_insert(boot_pgdir, p2, PGSIZE, PTE_U | PTE_W) == 0);
    assert((ptep = get_pte(boot_pgdir, PGSIZE, 0)) != NULL);
    assert(*ptep & PTE_U);
    assert(*ptep & PTE_W);
    assert(boot_pgdir[0] & PTE_U);
    assert(page_ref(p2) == 1);
    
    assert(page_insert(boot_pgdir, p1, PGSIZE, 0) == 0);
    assert(page_ref(p1) == 2);
    assert(page_ref(p2) == 0);
    assert((ptep = get_pte(boot_pgdir, PGSIZE, 0)) != NULL);
    assert(pte2page(*ptep) == p1);
    assert((*ptep & PTE_U) == 0);
    
    page_remove(boot_pgdir, 0x0);
    assert(page_ref(p1) == 1);
    assert(page_ref(p2) == 0);
    
    page_remove(boot_pgdir, PGSIZE);
    assert(page_ref(p1) == 0);
    assert(page_ref(p2) == 0);
    
    assert(page_ref(pde2page(boot_pgdir[0])) == 1);
    free_page(pde2page(boot_pgdir[0]));
    boot_pgdir[0] = 0;
    
    cprintf("check_pgdir() succeeded!\n");
}