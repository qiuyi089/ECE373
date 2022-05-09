#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
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
__used __section(__versions) = {
	{ 0x9de7765d, "module_layout" },
	{ 0x2142178f, "cdev_del" },
	{ 0x1facdfa4, "pci_unregister_driver" },
	{ 0xc8e2e1f4, "__pci_register_driver" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x19c48f0b, "cdev_add" },
	{ 0x18f0dc72, "cdev_init" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0xb44ad4b3, "_copy_to_user" },
	{ 0xa916b694, "strnlen" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x779a18af, "kstrtoll" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x37a0cba, "kfree" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0xc959d152, "__stack_chk_fail" },
	{ 0xd01519d3, "pci_ioremap_bar" },
	{ 0x61b2c4d5, "pci_set_master" },
	{ 0xd2d2c3f6, "pci_request_selected_regions" },
	{ 0xc4583d7a, "pci_enable_device_mem" },
	{ 0xe0dd827d, "pci_release_selected_regions" },
	{ 0xcae74629, "pci_select_bars" },
	{ 0xedc03953, "iounmap" },
	{ 0xc5850110, "printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "C4B8EBB4DDDB8B6DE450750");
