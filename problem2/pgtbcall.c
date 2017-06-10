#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/sched.h>
#include<linux/unistd.h>
#include<linux/mm.h>
#include<linux/slab.h>
#include<asm/uaccess.h>
MODULE_LICENSE("Dual BSD/GPL");
#define mysyscall1 380
#define mysyscall2 381
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

static int (*oldcall1)(void);
static int (*oldcall2)(void);

static int get_pagetable_layout(struct pagetable_layout_info __user* pgtbl_info,int size)
{
   unsigned long eleSize=sizeof(uint32_t); 
   uint32_t pgd_s=PGDIR_SHIFT;
   uint32_t pmd_s=PMD_SHIFT;
   uint32_t page_s=PAGE_SHIFT;
   //printk(KERN_INFO "pgd:%d\n",pgd_s);
   //printk(KERN_INFO "pmd:%d\n",pmd_s);
   //printk(KERN_INFO "pg:%d\n",page_s);

   //just check the size and copy them from kernel to userspace.
   if(size<eleSize)
       return size;
   else
   {
       if(copy_to_user(&(pgtbl_info->pgdir_shift),&pgd_s,eleSize))
           return -1;
       if(size<eleSize*2)
           return size;
       else
       {
           if(copy_to_user(&(pgtbl_info->pmd_shift),&pmd_s,eleSize))
               return -1;
           if(size<eleSize*3)
               return size;
           else if(copy_to_user(&(pgtbl_info->page_shift),&page_s,eleSize))
               return -1;
       }
   }
   return 0;
}

//info struct to construct fake pgd in kernel memory.
struct walk_info
{
    unsigned long pgtb_start;
    unsigned long fake_pgd;
    unsigned long * copied_pgd;
};

int cb4pgd(pmd_t *pgd,unsigned long addr,unsigned long end,struct mm_walk *walk)
{
    //printk(KERN_INFO"pgd:%08x",pgd);
    unsigned long pgdInd=pgd_index(addr);
    unsigned long pgdpg=pmd_page(*pgd);

    //get the physical frame number.
    unsigned long pfn=page_to_pfn((struct page *)pgdpg);
    if(pgd_none(*pgd)||pgd_bad(*pgd)||!pfn_valid(pfn)) return -EINVAL;
    int err=0;
    struct walk_info *copy_info=walk->private;
    //printk(KERN_INFO"pfn:%08X\n",pfn);

    struct vm_area_struct *user_vma=current->mm->mmap;
    if(!user_vma) return -EINVAL;
    struct vm_area_struct *vmp;
    //lock semaphore.
    down_write(&current->mm->mmap_sem);
    //remap the whole memory frame to userspace.
    err=remap_pfn_range(user_vma,copy_info->pgtb_start,pfn,PAGE_SIZE,user_vma->vm_page_prot);
    //unlock semaphore.
    up_write(&current->mm->mmap_sem);
    if(err)
    {
        printk(KERN_INFO"remap_pfn_range failed!\n");
        return err;
    }
    //construct temp fake pgd in kernel space.
    copy_info->copied_pgd[pgdInd]=copy_info->pgtb_start;
    copy_info->pgtb_start+=PAGE_SIZE;
    return 0;
}

int expose_page_table(pid_t pid,unsigned long fake_pgd,unsigned long fake_pmds,unsigned long page_table_addr,unsigned long begin_vaddr,unsigned long end_vaddr)
{
    if(begin_vaddr>=end_vaddr)
    {
        printk(KERN_INFO"Range fault!\n");
        return range_fault;
    }
    //get the pid struct.
    struct pid* pid_struct=find_get_pid(pid);
    if(!pid_struct)return no_such_pid;
    //get the task_struct with the target pid.
    struct task_struct *tarp=get_pid_task(pid_struct,PIDTYPE_PID);
    struct vm_area_struct *vmp;
    struct mm_walk walk={};
    struct walk_info copy_info={};
    int err=0;
    walk.mm=tarp->mm;
    //set callback function.
    walk.pgd_entry=&cb4pgd;
    copy_info.pgtb_start=page_table_addr;
    copy_info.fake_pgd=fake_pgd;
    //allocate memory for temp fake pgd in kernel space.
    copy_info.copied_pgd=kcalloc(PAGE_SIZE,sizeof(unsigned long),GFP_KERNEL);
    if(!copy_info.copied_pgd)
    {
        printk(KERN_INFO"kcalloc failed!\n");
        return allo_fail;
    }
    walk.private=&copy_info;

    //print some info in kernel, help us to use this system call.
    printk(KERN_INFO"The process with pid %d is %s",pid,tarp->comm);
    printk(KERN_INFO"vm area:\n");
    if(!(tarp->mm&&tarp->mm->mmap))
    {
        return no_mem_info;
    }
    for(vmp=tarp->mm->mmap;vmp;vmp=vmp->vm_next)
    {
        printk(KERN_INFO"%08X->%08X\n",vmp->vm_start,vmp->vm_end);
    }


    current->mm->mmap->vm_flags|=VM_SPECIAL;
    down_write(&tarp->mm->mmap_sem);
    //walk the page table recursively with our set callback function.
    err=walk_page_range(begin_vaddr,end_vaddr,&walk);
    up_write(&tarp->mm->mmap_sem);
    if(err)
    {
        printk(KERN_INFO"Walk fail!\n");
        return walk_fail;
    }
    //printk(KERN_INFO"err=%d\n",err);
    //copy the temp fake pgd to userspace.
    if(copy_to_user(fake_pgd,copy_info.copied_pgd,sizeof(unsigned long)*PAGE_SIZE)) return cp_fail;
    kfree(copy_info.copied_pgd);
    return 0;
}

static int addsyscall_init(void)
{
    long *syscall=(long*)0xc000d8c4;
    oldcall1=(int(*)(void))(syscall[mysyscall1]);
    oldcall2=(int(*)(void))(syscall[mysyscall2]);
    syscall[mysyscall1]=(unsigned long)get_pagetable_layout;
    syscall[mysyscall2]=(unsigned long)expose_page_table;
    printk(KERN_INFO "module load!\n");
    return 0;
}

static void addsyscall_exit(void)
{
    long *syscall=(long*)0xc000d8c4;
    syscall[mysyscall1]=(unsigned long)oldcall1;
    syscall[mysyscall2]=(unsigned long)oldcall2;
    printk(KERN_INFO "module exit!\n");
}

module_init(addsyscall_init);
module_exit(addsyscall_exit);
