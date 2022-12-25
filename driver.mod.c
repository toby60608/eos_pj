#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xc4162456, "module_layout" },
	{ 0x21522224, "gpiod_unexport" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xeb50e36b, "cdev_del" },
	{ 0xdb8c13c, "class_destroy" },
	{ 0x7ebe1016, "device_destroy" },
	{ 0xfe990052, "gpio_free" },
	{ 0x3ed00bdb, "gpiod_export" },
	{ 0x54f68cd1, "gpiod_direction_output_raw" },
	{ 0x47229b5c, "gpio_request" },
	{ 0x3b1038f3, "device_create" },
	{ 0xe72972aa, "__class_create" },
	{ 0xf99f641, "cdev_add" },
	{ 0x1bd8b5c, "cdev_init" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x250adc2a, "gpiod_get_raw_value" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0x21071cc2, "gpiod_set_raw_value" },
	{ 0x4bee6a03, "gpio_to_desc" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x56470118, "__warn_printk" },
	{ 0x1e6d26a8, "strstr" },
	{ 0x85df9b6c, "strsep" },
	{ 0xdd64e639, "strscpy" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x92997ed8, "_printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "65ABA91563846B19506B13C");
