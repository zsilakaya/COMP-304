
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>



pid_t pidID;
pid_t parentID;
module_param(pidID,int,0);


void DFS(struct task_struct *task)
{   
    struct task_struct *parent;
    struct task_struct *child;
    struct list_head *list;
    struct list_head *list2; 
    if(task->pid==pidID){
    	parent=task->parent;
        parentID=parent->pid;
	printk("name: %s, pid: [%d] ppid:[%d]", task->comm, task->pid,parentID);
 	printk(KERN_INFO "Siblings:\n");
	list_for_each(list2, &parent->children) {
        child = list_entry(list2, struct task_struct, sibling);
	printk(KERN_INFO "name: %s, pid: [%d]\n", child->comm, child->pid);
	}
    }
    	list_for_each(list, &task->children) {
        child = list_entry(list, struct task_struct, sibling);
        DFS(child);
    }
  
}
       
int init(void)
{      
       
	      printk(KERN_INFO "Loading Module\n");
	      DFS(&init_task);
	
           //  printk(KERN_INFO "Pid id is : %d ",pidID);
	   // printk(KERN_INFO "PARENT Pid id is : %d ",parentID);
	
      
      

       return 0;
}

void exit(void) {

	printk(KERN_INFO "Removing Module\n");
}

module_init(init);
module_exit(exit);

MODULE_LICENSE( "GPL");
MODULE_DESCRIPTION( "Exercise for COMP304");
MODULE_AUTHOR("Zeynep Sıla Kaya");
