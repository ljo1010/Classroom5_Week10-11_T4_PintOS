#ifndef VM_ANON_H
#define VM_ANON_H
#include "vm/vm.h"
#include "threads/vaddr.h"
struct page;
enum vm_type;
#define SECTORS_PER_PAGE (PGSIZE/DISK_SECTOR_SIZE)

struct anon_page {
    disk_sector_t swap_index;
    struct thread *thread;

    int swap_idx;
};

struct swap_anon{
    bool use;
    disk_sector_t sectors[8];
    struct page* page;  //필요한가?
    struct list_elem swap_elem;
};

void vm_anon_init (void);
bool anon_initializer (struct page *page, enum vm_type type, void *kva);

#endif
