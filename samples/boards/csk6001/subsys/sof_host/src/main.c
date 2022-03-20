/*
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <device.h>
#include <fs/fs.h>
#include <devicetree.h>
#if defined(CONFIG_SECOND_CORE_LOAD_BY_LITTLEFS)
#include <fs/littlefs.h>
#include <storage/flash_map.h>
#include <usb/usb_device.h>
#elif defined(CONFIG_SECOND_CORE_LOAD_BY_FATFS)
#include <ff.h>
#include <usb/usb_device.h>
#endif



#if defined(CONFIG_SECOND_CORE_LOAD_BY_LITTLEFS) || defined(CONFIG_SECOND_CORE_LOAD_BY_FATFS)

#if defined(CONFIG_SECOND_CORE_LOAD_BY_LITTLEFS)
/* Matches LFS_NAME_MAX */
#define MAX_PATH_LEN 255
#define PARTITION_NODE DT_NODELABEL(lfs1)
#if DT_NODE_EXISTS(PARTITION_NODE)
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
struct fs_mount_t *p_g_fs_mount = &FS_FSTAB_ENTRY(PARTITION_NODE);
#else /* PARTITION_NODE */
#error "Please declare the region of the file system first"
#endif /* PARTITION_NODE */
#elif defined(CONFIG_SECOND_CORE_LOAD_BY_FATFS)
#define FS_MOUNT_POINT "/NAND:"
struct fs_mount_t *p_g_fs_mount = NULL;
static struct fs_mount_t fs_mount_p;
static FATFS s_fat_fs;

#endif

static void s_fs_show(struct fs_mount_t *mp)
{
	unsigned int id = (uintptr_t)mp->storage_dev;
	struct fs_statvfs sbuf;
	const struct flash_area *pfa;
	int rc;

	rc = fs_statvfs(mp->mnt_point, &sbuf);
	if (rc < 0) {
		printk("FAIL: statvfs: %d\n", rc);
		goto out;
	}

	printk("%s: bsize = %lu ; frsize = %lu ;"
	       " blocks = %lu ; bfree = %lu\n",
	       mp->mnt_point, sbuf.f_bsize, sbuf.f_frsize, sbuf.f_blocks, sbuf.f_bfree);

	struct fs_dir_t dir;

	fs_dir_t_init(&dir);

	rc = fs_opendir(&dir, mp->mnt_point);
	printk("%s opendir: %d\n", mp->mnt_point, rc);

	while (rc >= 0) {
		struct fs_dirent ent = { 0 };

		rc = fs_readdir(&dir, &ent);
		if (rc < 0) {
			break;
		}
		if (ent.name[0] == 0) {
			printk("End of files\n");
			break;
		}
		printk("  %c %zu %s\n", (ent.type == FS_DIR_ENTRY_FILE) ? 'F' : 'D', ent.size,
		       ent.name);
	}

	(void)fs_closedir(&dir);
	printk("%s closedir\n", mp->mnt_point);
out:
	return;
}

static int s_fs_init(void)
{
	struct fs_mount_t *mp;
	int rc;

#if defined(CONFIG_SECOND_CORE_LOAD_BY_LITTLEFS)
	mp = p_g_fs_mount;
	/* Do not mount if auto-mount has been enabled */
#if !DT_NODE_EXISTS(PARTITION_NODE) ||                                                             \
	!(FSTAB_ENTRY_DT_MOUNT_FLAGS(PARTITION_NODE) & FS_MOUNT_FLAG_AUTOMOUNT)

	rc = fs_mount(mp);
	if (rc < 0) {
		printk("FAIL: mount id %" PRIuPTR " at %s: %d\n", (uintptr_t)mp->storage_dev,
		       mp->mnt_point, rc);
		return rc;
	}
	printk("%s mount: %d\n", mp->mnt_point, rc);
#else
	printk("%s automounted\n", mp->mnt_point);
#endif

#elif defined(CONFIG_SECOND_CORE_LOAD_BY_FATFS)
	if (p_g_fs_mount != NULL) {
		printk("The mount point<%s> busy!\n", p_g_fs_mount->mnt_point);
		return -1;
	}
	p_g_fs_mount = &fs_mount_p;
	mp = p_g_fs_mount;
	memset(mp, 0, sizeof(struct fs_mount_t));
	mp->type = FS_FATFS;
	/*mp->flags |= FS_MOUNT_FLAG_NO_FORMAT;*/
	mp->fs_data = &s_fat_fs;
	mp->mnt_point = FS_MOUNT_POINT;
	rc = fs_mount(mp);
	if (rc < 0) {
		printk("FAIL: mount id %" PRIuPTR " at %s: %d\n", (uintptr_t)mp->storage_dev,
		       mp->mnt_point, rc);
		return rc;
	}
	printk("%s mount: %d\n", mp->mnt_point, rc);
#endif
	s_fs_show(mp);

	return 0;
}
#endif

static void s_fs_mount(void)
{
	
	int ret;
//file system init
#if defined(CONFIG_SECOND_CORE_LOAD_BY_LITTLEFS) || defined(CONFIG_SECOND_CORE_LOAD_BY_FATFS)
	s_fs_init();
	ret = usb_enable(NULL);
	if (ret != 0) {
		printk("Failed to enable USB");
		return;
	}
#else
	#error "Please define file system type!"
#endif

}


extern void sof_domain(void);
extern void network_domain(void);

K_THREAD_STACK_DEFINE(sof_stack, 2048);
struct k_thread sof_thread;

void main(void)
{
	s_fs_mount();
	network_domain();
	k_msleep(1000);
	sof_domain();

	while (1) {
		k_msleep(1000);
	}
}

static int PSRAM_init(const struct device *unused)
{
	__attribute__((section("noinit"))) static uint32_t s_init =0;
	ARG_UNUSED(unused);
	if(0xaa55aa55 != s_init){
		printk("Init PSRAM!\n");
		PSRAM_Initialize();
		s_init= 0xaa55aa55;
	}
	
	return 0;
}

SYS_INIT(PSRAM_init, PRE_KERNEL_2, 0);
