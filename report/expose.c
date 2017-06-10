//expose_page_table implementation.
struct walk_info
{
    unsigned long pgtb_start;
    unsigned long fake_pgd;
    unsigned long * copied_pgd;
};

int cb4pgd(pmd_t *pgd,unsigned long addr,unsigned long end,struct mm_walk *walk)
{
    unsigned long pgdInd=pgd_index(addr);
    unsigned long pgdpg=pmd_page(*pgd);
    unsigned long pfn=page_to_pfn((struct page *)pgdpg);
    if(pgd_none(*pgd)||pgd_bad(*pgd)||!pfn_valid(pfn)) return -EINVAL;
    int err=0;
    struct walk_info *copy_info=walk->private;
    struct vm_area_struct *user_vma=current->mm->mmap;
    if(!user_vma) return -EINVAL;
    struct vm_area_struct *vmp;
    down_write(&current->mm->mmap_sem);
    err=remap_pfn_range(user_vma,copy_info->pgtb_start,pfn,PAGE_SIZE,user_vma->vm_page_prot);
    up_write(&current->mm->mmap_sem);
    if(err)
    {
        printk(KERN_INFO"remap_pfn_range failed!\n");
        return err;
    }
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
    struct pid* pid_struct=find_get_pid(pid);
    if(!pid_struct)return no_such_pid;
    struct task_struct *tarp=get_pid_task(pid_struct,PIDTYPE_PID);
    struct vm_area_struct *vmp;
    struct mm_walk walk={};
    struct walk_info copy_info={};
    int err=0;
    walk.mm=tarp->mm;
    walk.pgd_entry=&cb4pgd;
    copy_info.pgtb_start=page_table_addr;
    copy_info.fake_pgd=fake_pgd;
    copy_info.copied_pgd=kcalloc(PAGE_SIZE,sizeof(unsigned long),GFP_KERNEL);
    if(!copy_info.copied_pgd)
    {
        printk(KERN_INFO"kcalloc failed!\n");
        return allo_fail;
    }
    walk.private=&copy_info;
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
    err=walk_page_range(begin_vaddr,end_vaddr,&walk);
    up_write(&tarp->mm->mmap_sem);
    if(err)
    {
        printk(KERN_INFO"Walk fail!\n");
        return walk_fail;
    }
    if(copy_to_user(fake_pgd,copy_info.copied_pgd,sizeof(unsigned long)*PAGE_SIZE)) return cp_fail;
    kfree(copy_info.copied_pgd);
    return 0;
}
