//get_pagetable_layout implementation.
static int get_pagetable_layout(struct pagetable_layout_info __user* pgtbl_info,int size)
{
   unsigned long eleSize=sizeof(uint32_t); 
   uint32_t pgd_s=PGDIR_SHIFT;
   uint32_t pmd_s=PMD_SHIFT;
   uint32_t page_s=PAGE_SHIFT;
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
