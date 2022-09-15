#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/types.h>
#include<linux/slab.h>


struct birthday{
int day;
int month;
int year;
struct list_head list;
};

static LIST_HEAD(birthdaylist);

int addAndTraverseTheList_init(void){
	struct birthday *person1;
	person1 = kmalloc(sizeof(struct birthday), GFP_KERNEL);
	person1->day = 20;
	person1->month = 10;
	person1->year = 2000;
	INIT_LIST_HEAD(&person1->list);
	list_add_tail(&person1->list, &birthdaylist);

	struct birthday *person2;
	person2 = kmalloc(sizeof(struct birthday), GFP_KERNEL);
	person2->day = 28;
	person2->month = 11;
	person2->year = 2000;
	INIT_LIST_HEAD(&person2->list);
	list_add_tail(&person2->list, &birthdaylist);

	struct birthday *person3;
	person3 = kmalloc(sizeof(struct birthday), GFP_KERNEL);
	person3->day = 22;
	person3->month = 11;
	person3->year = 2000;
	INIT_LIST_HEAD(&person3->list);
	list_add_tail(&person3->list, &birthdaylist);

	struct birthday *person4;
	person4 = kmalloc(sizeof(struct birthday), GFP_KERNEL);
	person4->day = 24;
	person4->month = 12;
	person4->year = 1999;
	INIT_LIST_HEAD(&person4->list);
	list_add_tail(&person4->list, &birthdaylist);

	struct birthday *person5;
	person5 = kmalloc(sizeof(struct birthday), GFP_KERNEL);
	person5->day = 4;
	person5->month = 4;
	person5->year = 2000;
	INIT_LIST_HEAD(&person5->list);
	list_add_tail(&person5->list, &birthdaylist);

	printk(KERN_INFO,"Birthday List:\n");
	struct birthday *ptr;
	list_for_each_entry(ptr, &birthdaylist, list){
	/* on each iteration ptr points */
	/* to the next birthday struct */
        printk(KERN_INFO "Adding Person : %d/%d/%d",ptr->day,ptr->month,ptr->year);
}
	return 0;
}



void deleteAnd_exit(void) {

	printk(KERN_INFO "Removing Module\n");
	printk(KERN_INFO "Removing the elements\n");
	struct birthday *delPtr;
	struct birthday *next;
	list_for_each_entry_safe(delPtr,next,&birthdaylist,list){
		/* on each iteration ptr points */
		/* to the next birthday struct */
		printk(KERN_INFO "Deleting Person : %d/%d/%d",delPtr->day,delPtr->month,delPtr->year);
		list_del(&delPtr->list);
		kfree(delPtr);
}
}


module_init(addAndTraverseTheList_init);
module_exit(deleteAnd_exit);
MODULE_LICENSE( "GPL");
MODULE_DESCRIPTION( "Exercise for COMP304");
MODULE_AUTHOR("Zeynep Sıla Kaya");

