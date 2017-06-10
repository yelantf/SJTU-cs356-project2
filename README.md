# SJTU-cs356-project2
Operating system project 2(Fan Wu  2017 Spring): Android Memory Managment.

### ./CS356-Project2.pdf                  
project2 slides.
### ./CS356prj2.pdf						                
project2 doc.

## Problem 1:
nothing to submit.

## Problem 2:
### ./problem2/pgtbcall.c 					            
Two system calls module source code.
### ./problem2/Makefile					                
The makefile to compile the module.
### ./problem2/VATranslate/jni/VATranslate.c		    
The test program VATranslate's source code.
### ./problem2/VATranslate/jni/Android.mk			    
The android makefile of the test program VATranslate.
### ./problem2/kernelFile/pagewalk.c			        
Modified kernel file(goldfish/mm/pagewalk.c). Add '#include <linux/export.h>' in Line1, add 'EXPORT_SYMBOL(walk_page_range);' in Line252.
### ./problem2/kernelFile/mm.h				            
Modified kernel file(goldfish/include/linux/mm.h). Add 'extern' before previous Line945.

## Problem 3:
### ./problem3/jni/vm_inspector.c				        
The program vm_inspector's source code.
### ./problem3/jni/Android.mk				           
The android makefile of the vm_inspector program.

## Problem 4:
### ./problem4/mm_types.h					            
Modified kernel file(goldfish//include/linux/mm_types.h). Add a variable to page struct.
### ./problem4/swap.c					                
Modified kernel file(goldfish/mm/swap.c). Change the definition of function 'void mark_page_accessed(struct page *page)'.
### ./problem4/vmscan.c					                
Modified kernel file(goldfish/mm/vmscan.c). Change the definition of function 'shrink_active_list' and function 'page_check_references'.


## Report:
### ./report.pdf
### ./report/*						                    
report latex file and some resourses.
