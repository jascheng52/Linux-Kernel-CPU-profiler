#include <linux/module.h> /* Needed by all modules */
#include <linux/kernel.h> /* KERN_INFO */
#include <linux/init.h> /* Init and exit macros */
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kprobes.h>

static int perftop_show(struct seq_file *m, void *v);
static int perftop_open(struct inode *inode, struct  file *file);
static int entry_pick_next_fair(struct kretprobe_instance *ri, struct pt_regs *regs);
static int ret_pick_next_fair(struct kretprobe_instance *ri, struct pt_regs *regs);




atomic_t pre_count = ATOMIC_INIT(0);
atomic_t post_count = ATOMIC_INIT(0);
atomic_t context_switch_count = ATOMIC_INIT(0);
struct my_data {
	volatile unsigned long prev_task;
};

static int perftop_show(struct seq_file *m, void *v) 
{
    // seq_printf(m, "Hello World\n");
    seq_printf(m, "pre_count %d\n", pre_count.counter);
    seq_printf(m, "postcount %d\n", post_count.counter);
    seq_printf(m, "context_switch_count %d\n", context_switch_count.counter);
    return 0;
}


static int perftop_open(struct inode *inode, struct  file *file) 
{
  return single_open(file, perftop_show, NULL);
}

static const struct proc_ops perftop_fops = {
  .proc_open = perftop_open,
  .proc_read = seq_read,
  .proc_lseek = seq_lseek,
  .proc_release = single_release,
};


static int entry_pick_next_fair(struct kretprobe_instance *ri, struct pt_regs *regs)
{
  // struct task_struct *prev ;
  struct my_data *data = (struct my_data *)ri->data;
  atomic_inc(&pre_count);
  data->prev_task = regs->si;
  // printk(KERN_INFO "PREV TASK: %d  %p\n ", prev->pid, prev);
  return 0;
}
NOKPROBE_SYMBOL(entry_pick_next_fair);

static int ret_pick_next_fair(struct kretprobe_instance *ri, struct pt_regs *regs)
{
  struct task_struct *next ;
  struct my_data *data = (struct my_data *)ri->data;
  unsigned long next_task;
  atomic_inc(&post_count);
  next_task = regs->ax;
  next = (struct task_struct *) next_task;
  if(next != NULL && next_task !=  data->prev_task)
    atomic_inc(&context_switch_count);

  return 0;
}
NOKPROBE_SYMBOL(ret_pick_next_fair);


static struct kretprobe perftop_kretprobe = 
{
	.handler		= ret_pick_next_fair,
	.entry_handler		= entry_pick_next_fair,
	.data_size		= sizeof(struct my_data),
	/* Probe up to 20 instances concurrently. */
	// .maxactive		= 20,
};

static int __init lkp_init(void)
{
    int ret;
    proc_create("perftop", 0, NULL, &perftop_fops);
    perftop_kretprobe.kp.symbol_name = "pick_next_task_fair";
    ret = register_kretprobe(&perftop_kretprobe);
    
    if(ret < 0) 
    {
		  pr_err("register_kretprobe failed, returned %d\n", ret);
		return -1;
	  }
	  return 0;

}

static void __exit lkp_exit(void)
{
    remove_proc_entry("perftop", NULL);
    unregister_kretprobe(&perftop_kretprobe);
    printk(KERN_INFO "Module exiting ...\n");
}




module_init(lkp_init);
module_exit(lkp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jason Cheng <jason.cheng.1@stonybrook.edu>");
MODULE_DESCRIPTION("perftops module");