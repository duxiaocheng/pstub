#include "stub.h"

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <string.h>
#include <assert.h>

static void dumpHex(void *_pdat, size_t length)
{
    uint8_t *pdat = (uint8_t*)_pdat;
    for (int i = 0; i < (int)length; i += 16) {
        fprintf(stdout, "0x%016x: ", pdat);
        for (int j = 0; j < 16; j++) {
            fprintf(stdout, "%02x ", *pdat++);
        }
        pdat -= 16;
        fprintf(stdout, "  ");
        for (int k = 0; k < 16; k++) {
            fprintf(stdout, "%c", *pdat > 31 && *pdat <= 'z' ? *pdat:'.');
            pdat++;
        }
        fprintf(stdout, "\n");
        if(i != 0 && i % 512 == 0)
        {
            fprintf(stdout, "\n");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

typedef struct list_head
{
    struct list_head *next, *prev;

}list_head_t;

#define LIST_HEAD_INIT(name) { &(name),&(name) }
#define LIST_HEAD(name) \
struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
(ptr)->next = (ptr);(ptr)->prev = (ptr);\
} while (0)

static __inline__ void __list_add(struct list_head *new,
    struct list_head *prev,
    struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static __inline__ void list_add(struct list_head *new, struct list_head *head)
{
    __list_add(new, head, head->next);
}

static __inline__ void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

static __inline__ void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

#define list_entry(ptr,type,member) \
   ((type*)((char*)(ptr)-(unsigned long)(&((type*)0)->member)))
#define list_for_each(pos,head) \
for(pos=(head)->next;pos!=(head);pos=pos->next)

static __inline__ int list_count(struct list_head *head)
{
    struct list_head *pos;
    int count = 0;
    list_for_each(pos, head)
    {
        count++;
    }
    return count;
}

///////////////////////////////////////////////////////////////////////////////

LIST_HEAD(head);
struct stub
{
    struct list_head node;
    char             desc[256];
    void             *orig_f;
    void             *stub_f;
    unsigned int     stubpath;
    unsigned int     old_flg;
    unsigned char    assm[5];
};

int init_lock_flag = 0;
void initlock()
{
}

void lock()
{
}

void unlock()
{
}

static int set_mprotect(struct stub* pstub)
{
    int pagesize = sysconf(_SC_PAGE_SIZE);
    if (-1 == pagesize) {
        printf("pagesize:%d\n", pagesize);
        assert(pagesize != -1);
    }
    void *p = (void*)((long)pstub->orig_f& ~(pagesize - 1));
    int ret = mprotect(p, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC);
    if (0 != ret) {
        assert(ret == 0);
        printf("mprotect return:%d\n", ret);
    }
    return ret;
}

static int set_asm_jmp(struct stub* pstub)
{
    unsigned int offset;
    memcpy(pstub->assm, pstub->orig_f, sizeof(pstub->assm));
    *((char*)pstub->orig_f) = 0xE9;
    offset = (unsigned int)((long)pstub->stub_f - ((long)pstub->orig_f + 5));
    *((unsigned int*)((char*)pstub->orig_f + 1)) = offset;
    return 0;
}

static void restore_asm(struct stub* pstub)
{
    memcpy(pstub->orig_f, pstub->assm, sizeof(pstub->assm));
	// TODO: restore the memory protect
}

int install_stub(void *orig_f, void *stub_f, char *desc)
{
    struct stub *pstub;
    pstub = (struct stub*)malloc(sizeof(struct stub));
    pstub->orig_f = orig_f;
    pstub->stub_f = stub_f;
    do
    {
        if (set_mprotect(pstub))
        {
            break;
        }
        if (set_asm_jmp(pstub))
        {
            break;
        }

        if (desc)
        {
            strncpy(pstub->desc, desc, sizeof(pstub->desc));
        }
        lock();
        list_add(&pstub->node, &head);
        unlock();
        return 0;
    } while (0);

    free(pstub);
    return -1;
}

int uninstall_stub(void* stub_f)
{
    struct stub *pstub;
    struct list_head *pos;
    list_for_each(pos, &head)
    {
        pstub = list_entry(pos, struct stub, node);
        if (pstub->stub_f == stub_f)
        {
            restore_asm(pstub);
            lock();
            list_del(&pstub->node);
            unlock();
            free(pstub);
            return 0;
        }
    }
    return -1;
}
