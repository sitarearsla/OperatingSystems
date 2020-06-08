#include <linux/init.h>

#include <linux/kernel.h>

#include <linux/module.h>

#include <linux/list.h>

#include <linux/slab.h>

struct birthday{

   int day;

   int month;

   int year;

   struct list_head list;

};

static LIST_HEAD(b_list);

/* This function is called when the module is loaded. */

int simple_init(void){

printk(KERN_INFO "Loading Module\n");


struct birthday *b1;

struct birthday *b2;

struct birthday *b3;

struct birthday *b4;

struct birthday *b5;

b1=kmalloc(sizeof(*b1),GFP_KERNEL);
b1->year=1996;
b1->day=10;
b1->month=10;



INIT_LIST_HEAD(&b1->list);

b2=kmalloc(sizeof(*b2),GFP_KERNEL);

b2->year=1996;
b2->day=11;
b2->month=10;

INIT_LIST_HEAD(&b2->list);

b3=kmalloc(sizeof(*b3),GFP_KERNEL);

b3->year=1996;
b3->day=19;
b3->month=10;

INIT_LIST_HEAD(&b3->list);

b4=kmalloc(sizeof(*b4),GFP_KERNEL);

b4->year=1996;
b4->day=22;
b4->month=10;

INIT_LIST_HEAD(&b4->list);

b5=kmalloc(sizeof(*b5),GFP_KERNEL);

b5->year=1996;
b5->day=20;
b5->month=10;

INIT_LIST_HEAD(&b5->list);

list_add_tail(&b1->list,&b_list);
list_add_tail(&b2->list,&b_list);
list_add_tail(&b3->list,&b_list);
list_add_tail(&b4->list,&b_list);
list_add_tail(&b5->list,&b_list);

struct birthday *ptr;

list_for_each_entry(ptr,&b_list,list){

	printk(KERN_INFO "Birthday: %d-%d-%d\n",ptr->day,ptr->month,ptr->year);
}

return 0;

}

void simple_exit(void){

printk(KERN_INFO "Removing Module\n");


struct birthday *ptr, *next;

list_for_each_entry_safe(ptr,next,&b_list,list){

list_del(&ptr->list);

kfree(ptr);

}

}


module_init(simple_init);

module_exit(simple_exit);


MODULE_LICENSE("GPL");

MODULE_DESCRIPTION("Simple Module");

MODULE_AUTHOR("SGG");
