#include<stdio.h>
#include<stdlib.h>
#include<sys/mman.h>
#define pgd_index(va,loinfo) ((va)>>loinfo.pgdir_shift)
#define pte_index(va,loinfo) (((va)>>loinfo.page_shift)&((1<<(loinfo.pmd_shift-loinfo.page_shift))-1))
#define no_such_pid 1
#define no_mem_info 2
#define allo_fail   3
#define range_fault 4
#define walk_fail   5
#define cp_fail     -1

struct pagetable_layout_info{
    uint32_t pgdir_shift;
    uint32_t pmd_shift;
    uint32_t page_shift;
};

int main(int argc,char **argv)
{
    pid_t pid;
    unsigned long begin_vaddr;
    unsigned long end_vaddr;
    unsigned long *table_addr,*fake_pgd_addr;
    struct pagetable_layout_info loinfo;
    int err=0;
    unsigned long page_size;
    unsigned long rd_begin,rd_end;
    unsigned long mask;
    unsigned long page_nums;
    unsigned long tb_num;
    unsigned long pfn;
    unsigned long pgd_ind,phy_addr;
    unsigned long *phy_base;

    //check the arguments.
    if(argc!=4)
    {
        printf("Wrong arguments!\n");
        printf("Usage: ./vm_inspector pid begin_vaddr end_vaddr\n");
        return -1;
    }
    pid=atoi(argv[1]);
    begin_vaddr=strtoul(argv[2],NULL,16);
    end_vaddr=strtoul(argv[3],NULL,16);

    //get the layout.
    err=syscall(380,&loinfo,4*3);
    if(err<0)
    {
        printf("copy to user failed!\n");
        return -1;
    }
    err=0;
    err=syscall(380,&loinfo,4*3);
    if(err<0)
    {
        printf("copy to user failed!\n");
        return -1;
    }
    err=0;

    //page size.
    page_size=1<<(loinfo.page_shift);
    //mask is used to mask the lower bit of page table entry.
    mask=page_size-1;

    //round the begin_vaddr and end_vaddr to be aligned with page size.
    rd_begin=begin_vaddr&~mask;
    rd_end=(end_vaddr+mask)&~mask;

    //printf("rounded begin,rounded end:%08lx,%08lx\n",rd_begin,rd_end);

    //calculate needed pages.
    page_nums=pgd_index(rd_end-1,loinfo)-pgd_index(rd_begin,loinfo)+1;
    //printf("page nums=%d\n",page_nums);

    //allocate enough memory space.
    table_addr=mmap(NULL,page_size*page_nums,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    fake_pgd_addr=malloc(sizeof(unsigned long)*page_size);

    if(!table_addr||!fake_pgd_addr)
    {
        printf("allocate memory failed!\n");
        return -1;
    }

    err=syscall(381,pid,fake_pgd_addr,0,table_addr,begin_vaddr,end_vaddr);
    //error handler.
    switch(err)
    {
        case no_such_pid:
        printf("Process with pid=%d doesn't exist.\n",pid);
        return -1;
        case no_mem_info:
        printf("Target process has no virtual memory infomation.\n");
        return -1;
        case allo_fail:
        printf("allocate memory failed.\n");
        return -1;
        case range_fault:
        printf("virtual address range fault.\n");
        return -1;
        case walk_fail:
        printf("walk_page_range failed.\n");
        return -1;
        case cp_fail:
        printf("copy to user failed!\n");
        return -1;
    }

    //print page number and corresponding frame number.
    printf("page number\t\t\tframe number\n");
    for(tb_num=rd_begin>>loinfo.page_shift;tb_num<rd_end>>loinfo.page_shift;++tb_num)
    {
        pgd_ind=pgd_index(tb_num<<loinfo.page_shift,loinfo);
        phy_base=fake_pgd_addr[pgd_ind];
        if(phy_base)
        {
            phy_addr=phy_base[pte_index(tb_num<<loinfo.page_shift,loinfo)];
            pfn=phy_addr>>loinfo.page_shift;
            if(pfn)
            {
                printf("0x%08lx\t\t\t0x%08lx\n",tb_num,pfn);
            }
        }
    }

    //free memory space.
    free(fake_pgd_addr);
    munmap(table_addr,page_size*page_nums);
    return 0;
}
