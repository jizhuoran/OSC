#include "list.h"
#include "default_pmm.h"
#include "pmm.h"
#include "memlayout.h"

free_area_t free_area;

#define free_list (free_area.free_list)
#define nr_free (free_area.nr_free)


static void default_init(void) {
    list_init(&free_list);
    nr_free = 0;
}

static void default_init_memmap(struct Page *base, size_t n) {
    assert( n>0 );
    struct Page *p = base;
    for (; p < (base + n); ++p) {
        assert(PageReserved(p));
        assert(PageProperty(p)); //检查有没有已经被分配或者reserved.
        SetPageProperty(p);//把property的bit设置，表示已经被加入
        p->flags = 0;//设置flags为0
        p->property = 0;//表示有多少空闲页，每一个都设置成0，但是第一个要设置成n
        set_page_ref(p, 0);//因为现在还没有被引用，所以设置成0
        list_add_before(&free_list, &(p->page_link));
        //free_list==>base free_list ==>base (base的下一个) free_list
        //因为是双向链表，所以这里放到前面是对的
    }
    nr_free += n;
    base->property = n;
}

static struct Page* default_alloc_pages(size_t n){
    assert(n > 0);//检测要分配的大不大于0
    assert(n < nr_free);//看有没有足够的free_page
    list_entry_t *entry = &free_list,*temp = NULL;
    struct Page *p = NULL;
    while((entry = list_next(entry)) != &free_list) {//遍历list，看有没有合适的块
        p = le2page(entry,page_link);
        if(p->property >= n) { //找到了合适的块
            int i;
            for(i = 0; i < n; ++i){//给每一个设置好参数，并从list中删除
                temp = list_next(entry);
                struct Page *pp = le2page(entry, page_link);
                SetPageReserved(pp);
                ClearPageProperty(pp);
                list_del(entry);
                entry = temp;
            }
            
            if(p->property > n) {//如果这个块比n大，那么剩下的块的第一个page变成了新的头
                struct Page *remain = le2page(temp, page_link);
                remain->property = (p->property) - n;
            }
            nr_free -= n;//总的free的page数减去n
            return p;
        }
    }
    return NULL;
}

static void default_free_pages(struct Page *base, size_t n){
    assert(n > 0);
    list_entry_t *entry = &free_list;
    struct Page *p = NULL;
    while ((entry = list_next(entry)) != &free_list) {//找到base的位置
        p = le2page(entry, page_link);
        if(p>base) break;
    }
    struct Page *page_to_be_add = base;
    for (; page_to_be_add < (base + n);++page_to_be_add) {//一个个的把页加到list里
        list_add_before(entry, &(page_to_be_add->page_link));
    }
    //进行一些参数设置
    ClearPageProperty(base);
    ClearPageReserved(base);
    base->flags = 0;
    set_page_ref(base, 0);
    base->property = n;
    
    //如果base跟他后面的块可以合并
    if(p->property) {
        base->property += p->property;
        p->property = 0;
    }
    
    //如果base跟她（为了政治正确）前面的块可以合并
    entry = list_prev(entry);//先得到前面的element
    p = le2page(entry, page_link);//的页
    if(entry != &free_list && p+1 == base) {//看他是不是free_list，如果不是，看他是不是正好在base前面
        while (entry != &free_list) {//找到前面块的头
            if(p->property) {
                p->property += base->property;
                base->property = 0;
            }
            entry = list_prev(entry);
            p = le2page(entry, page_link);
            break;
        }
    }
    nr_free += n;
    return;
}

static size_t default_nr_free_pages(void) {
    return nr_free;
}

static void
basic_check(void) {
    struct Page *p0, *p1, *p2;
    p0 = p1 = p2 = NULL;
    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);
    
    assert(p0 != p1 && p0 != p2 && p1 != p2);
    assert(page_ref(p0) == 0 && page_ref(p1) == 0 && page_ref(p2) == 0);
    
    assert(page2pa(p0) < npage * PGSIZE);
    assert(page2pa(p1) < npage * PGSIZE);
    assert(page2pa(p2) < npage * PGSIZE);
    
    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));
    
    unsigned int nr_free_store = nr_free;
    nr_free = 0;
    
    assert(alloc_page() == NULL);
    
    free_page(p0);
    free_page(p1);
    free_page(p2);
    assert(nr_free == 3);
    
    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);
    
    assert(alloc_page() == NULL);
    
    free_page(p0);
    assert(!list_empty(&free_list));
    
    struct Page *p;
    assert((p = alloc_page()) == p0);
    assert(alloc_page() == NULL);
    
    assert(nr_free == 0);
    free_list = free_list_store;
    nr_free = nr_free_store;
    
    free_page(p);
    free_page(p1);
    free_page(p2);
}

// LAB2: below code is used to check the first fit allocation algorithm (your EXERCISE 1)
// NOTICE: You SHOULD NOT CHANGE basic_check, default_check functions!
static void
default_check(void) {
    int count = 0, total = 0;
    list_entry_t *le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        assert(PageProperty(p));
        count ++, total += p->property;
    }
    assert(total == nr_free_pages());
    
    basic_check();
    
    struct Page *p0 = alloc_pages(5), *p1, *p2;
    assert(p0 != NULL);
    assert(!PageProperty(p0));
    
    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));
    assert(alloc_page() == NULL);
    
    unsigned int nr_free_store = nr_free;
    nr_free = 0;
    
    free_pages(p0 + 2, 3);
    assert(alloc_pages(4) == NULL);
    assert(PageProperty(p0 + 2) && p0[2].property == 3);
    assert((p1 = alloc_pages(3)) != NULL);
    assert(alloc_page() == NULL);
    assert(p0 + 2 == p1);
    
    p2 = p0 + 1;
    free_page(p0);
    free_pages(p1, 3);
    assert(PageProperty(p0) && p0->property == 1);
    assert(PageProperty(p1) && p1->property == 3);
    
    assert((p0 = alloc_page()) == p2 - 1);
    free_page(p0);
    assert((p0 = alloc_pages(2)) == p2 + 1);
    
    free_pages(p0, 2);
    free_page(p2);
    
    assert((p0 = alloc_pages(5)) != NULL);
    assert(alloc_page() == NULL);
    
    assert(nr_free == 0);
    nr_free = nr_free_store;
    
    free_list = free_list_store;
    free_pages(p0, 5);
    
    le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        count --, total -= p->property;
    }
    assert(count == 0);
    assert(total == 0);
}

const struct pmm_manager default_pmm_manager = {
    .name = "default_pmm_manager",
    .init = default_init,
    .init_memmap = default_init_memmap,
    .alloc_pages = default_alloc_pages,
    .free_pages = default_free_pages,
    .nr_free_pages = default_nr_free_pages,
    .check = default_check,
};