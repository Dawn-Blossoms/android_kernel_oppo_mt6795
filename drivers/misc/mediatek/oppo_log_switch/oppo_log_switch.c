/*
 * VENDOR_EDIT
 * Copyright (C) 2011 OPPO, Inc.
 * Author: hsy@oppo.com
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>




int oppo_kernel_log = 1;
int oppo_android_log = 1;
int oppo_uart_log = 1;


EXPORT_SYMBOL(oppo_kernel_log);
EXPORT_SYMBOL(oppo_android_log);
EXPORT_SYMBOL(oppo_uart_log);

static ssize_t kernel_log_switch_write(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	char tmp[2] = {0, 0};
	int ret;
	
	if (count > 2)
		return -EINVAL;
	
	ret = copy_from_user(tmp, buf, 2);

	if ('1' == tmp[0])
		oppo_kernel_log = 1;
	else if ('0' == tmp[0])
		oppo_kernel_log = 0;
	else
		return -EINVAL;	

	return count;
	
}


static ssize_t android_log_switch_write(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	char tmp[2] = {0, 0};
	int ret;
	
	if (count > 2)
		return -EINVAL;
	
	ret = copy_from_user(tmp, buf, 2);

	if ('1' == tmp[0])
		oppo_android_log = 1;
	else if ('0' == tmp[0])
		oppo_android_log = 0;
	else
		return -EINVAL;	

	return count;
	
}


static ssize_t uart_log_switch_write(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	char tmp[2] = {0, 0};
	int ret;
	
	if (count > 2)
		return -EINVAL;
	
	ret = copy_from_user(tmp, buf, 2);

	if ('1' == tmp[0])
	{
		oppo_uart_log = 1;
	}
	else if ('0' == tmp[0])
	{
		oppo_uart_log = 0;
	}
	else
		return -EINVAL;	

	return count;
	
}


static ssize_t kernel_log_switch_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	char page[8]; 
	char *p = page;
	int len = 0; 

	p += sprintf(p, "%d\n", oppo_kernel_log);

	len = p - page;
	if (len > *pos)
		len -= *pos;
	else
		len = 0;
	if (copy_to_user(buf,page,len < count ? len  : count))
		return -EFAULT;
	*pos = *pos + (len < count ? len  : count);
	return len < count ? len  : count;
}

static ssize_t android_log_switch_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	char page[8]; 
	char *p = page;
	int len = 0; 

	p += sprintf(p, "%d\n", oppo_android_log);

	len = p - page;
	if (len > *pos)
		len -= *pos;
	else
		len = 0;
	if (copy_to_user(buf,page,len < count ? len  : count))
		return -EFAULT;
	*pos = *pos + (len < count ? len  : count);
	return len < count ? len  : count;
}

static ssize_t uart_log_switch_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	char page[8]; 
	char *p = page;
	int len = 0; 

	p += sprintf(p, "%d\n", oppo_uart_log);

	len = p - page;
	if (len > *pos)
		len -= *pos;
	else
		len = 0;
	if (copy_to_user(buf,page,len < count ? len  : count))
		return -EFAULT;
	*pos = *pos + (len < count ? len  : count);
	return len < count ? len  : count;
}

static const struct file_operations kernel_log_switch = {
	.write		= kernel_log_switch_write,
	.read		= kernel_log_switch_read,
};

static const struct file_operations android_log_switch = {
	.write		= android_log_switch_write,
	.read		= android_log_switch_read,
};

static const struct file_operations uart_log_switch = {
	.write		= uart_log_switch_write,
	.read		= uart_log_switch_read,
};

static int __init log_switch_init(void)
{
	struct proc_dir_entry* switch_dir = NULL;
	switch_dir = proc_mkdir_mode("oppo_log_switch", 0777, NULL);
	proc_create("kernel", 0666, switch_dir, &kernel_log_switch);
	proc_create("android", 0666, switch_dir, &android_log_switch);
	proc_create("uart", 0666, switch_dir, &uart_log_switch);
	return 0;
}
module_init(log_switch_init);
