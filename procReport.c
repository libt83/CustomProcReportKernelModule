/**
 *  Brandon Semba and James(Feng) Yang
 *  TCSS 422 Assignment 2
 *  11/15/2016
 *
 *  Program is an implementation of a kernel module designed to get
 *	process information from the kernel in order to create a proc file
 *  that can then be used to dynamically generate output via the 
 *  command line.
 */
 
#include <linux/list.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

/* Struct to house the information related to a process */
typedef struct PROC_DATA
{
	int parent_pid;
	char *parent_name;
	int num_of_children;
	int first_child_pid;
	char *first_child_name;
	struct list_head list;
}PROC_DATA_S;


/* Global pointer to keep track of data */
static PROC_DATA_S* globalData;

/* Function prototype declarations */
int proc_init(void);
void proc_cleanup(void);
int generate_proc_data(PROC_DATA_S *proc_data);
int proc_report_show(struct seq_file *m, void *v);
int proc_report_open(struct inode *inode, struct file *file);

/**
 * Gets relevant data for processes out of the task list
 */
int generate_proc_data(PROC_DATA_S *proc_data)
{
	struct task_struct *task, *first_child;
	struct list_head *list;

	// iterate over all processes in the task list
	for_each_process(task)
	{
		int child_counter = 0;
		// creates new node for linked list containing process information and adds it to list
		PROC_DATA_S *new_struct_node;
		new_struct_node = kmalloc(sizeof(PROC_DATA_S), GFP_KERNEL);
		list_add_tail(&new_struct_node->list, &proc_data->list);

		new_struct_node->parent_pid = task->pid;
		new_struct_node->parent_name = task->comm;

		// tests whether current process has at least 1 child	
		if(!list_empty(&task->children))
		{
            // gets the first child of the current process
			first_child = list_first_entry(&task->children, struct task_struct, sibling);
			new_struct_node->first_child_pid = first_child->pid;
			new_struct_node->first_child_name = first_child->comm;

			// iterate over each processes' children list to get a count for total children
			list_for_each(list, &task->children)
			{
				child_counter++;
			}
		}
		new_struct_node->num_of_children = child_counter;
	
	}
	return 0;
}

/* Function that generate process report to proc_report file */
int proc_report_show(struct seq_file *m, void *v)
{
	PROC_DATA_S *ptr, *next;
	PROC_DATA_S *procList = globalData;
	seq_printf(m,"PROCESS REPORT: \n");
	list_for_each_entry_safe(ptr, next, &procList->list, list)
	{
		if (ptr->num_of_children == 0){
			seq_printf(m,"PROCESS ID=%d Name=%s *No Children\n", ptr->parent_pid, ptr->parent_name);
		} else {
			seq_printf(m,"PROCESS ID=%d Name=%s number_of_kids=%d first_child_pid=%d first_child_name=%s\n", ptr->parent_pid, ptr->parent_name, ptr->num_of_children, ptr->first_child_pid, ptr->first_child_name);
		}		
	}
    return 0;
}

int proc_report_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_report_show, NULL);
}

static const struct file_operations proc_report_fops = {
    .owner      = THIS_MODULE,
    .open       = proc_report_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
};

/* Intializes the kernel module */
int proc_init(void)
{
	// struct list_head *node;
	PROC_DATA_S *procList = kmalloc(sizeof(PROC_DATA_S), GFP_KERNEL);
	INIT_LIST_HEAD(&procList->list);
	globalData = procList; // set global list to the generated list
	
	generate_proc_data(procList); // iterate through process list, save info to my data structure for later use
	proc_create("proc_report", 0, NULL, &proc_report_fops); // generate proc_report file

	return 0;
}

/* Removes the kernel module and cleans up allocated memory */
void proc_cleanup(void)
{
	// free my global data structure
  	PROC_DATA_S* ptr, *next;
	remove_proc_entry("proc_report",NULL);  // remove proc_report file

	list_for_each_entry_safe(ptr, next, &globalData->list, list)
	{
				
		list_del(&ptr->list);
		kfree(ptr);
	}
}


MODULE_LICENSE("GPL");
module_init(proc_init);
module_exit(proc_cleanup);
