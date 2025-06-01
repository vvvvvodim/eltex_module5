#include <linux/module.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <linux/console_struct.h>
#include <linux/vt_kern.h>
#include <linux/timer.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/slab.h>

MODULE_DESCRIPTION("Keyboard LEDs control with blinking via sysfs");
MODULE_LICENSE("GPL");

#define BLINK_DELAY   HZ/5
#define RESTORE_LEDS  0xFF
#define RESTORE_LEDS    0xFF

static struct tty_driver *my_driver;
static struct timer_list my_timer;
static int blink_pattern = 0; //(0-7)
static int current_led_state = 0;

static struct kobject *kobject_example;

static void update_leds(int state) {
    if (my_driver && my_driver->ops && my_driver->ops->ioctl) {
        (my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, state);
    }
}

static void blink_timer_func(struct timer_list *unused) {
    if (current_led_state == blink_pattern) {
        current_led_state = 0; 
    } else {
        current_led_state = blink_pattern;
    }
    
    update_leds(current_led_state);
    my_timer.expires = jiffies + BLINK_DELAY;
    add_timer(&my_timer);
}

static ssize_t foo_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", blink_pattern);
}

static ssize_t foo_store(struct kobject *kobj, struct kobj_attribute *attr, 
                         const char *buf, size_t count) {
    unsigned int val;
    
    if (sscanf(buf, "%u", &val) != 1 || val > 7)
        return -EINVAL;

    blink_pattern = val;
    return count;
}

static struct kobj_attribute foo_attribute = __ATTR(foo, 0664, foo_show, foo_store);

static int __init kbleds_init(void) {
    int ret;

    printk(KERN_INFO "kbleds: initializing\n");

    my_driver = vc_cons[fg_console].d->port.tty->driver;
    if (!my_driver) {
        printk(KERN_ERR "kbleds: cannot get tty driver\n");
        return -ENODEV;
    }

    kobject_example = kobject_create_and_add("kobject_example", kernel_kobj);
    if (!kobject_example) {
        printk(KERN_ERR "kbleds: failed to create sysfs kobject\n");
        return -ENOMEM;
    }

    ret = sysfs_create_file(kobject_example, &foo_attribute.attr);
    if (ret) {
        printk(KERN_ERR "kbleds: failed to create sysfs file\n");
        kobject_put(kobject_example);
        return ret;
    }


    timer_setup(&my_timer, blink_timer_func, 0);
    my_timer.expires = jiffies + BLINK_DELAY;
    add_timer(&my_timer);;

    return 0;
}

static void __exit kbleds_cleanup(void) {
    printk(KERN_INFO "kbleds: unloading\n");
    
    del_timer_sync(&my_timer);

    update_leds(RESTORE_LEDS);
    
    sysfs_remove_file(kobject_example, &foo_attribute.attr);
    kobject_put(kobject_example);
}

module_init(kbleds_init);
module_exit(kbleds_cleanup);