#include <linux/fs.h>
#include <linux/version.h>
#include <linux/slab.h>
#include "file_proxy.h"

/*
 * KernelSU: fixed file_proxy for compatibility across kernel versions.
 * Supports remap/clone/dedupe variants safely.
 */

/*
 * 4.14 → tidak punya remap_file_range()
 * 4.15–4.18 → ada remap_file_range() versi lama
 * >=4.19 → API berubah → clone_file_range/dedupe_file_range
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)

/* =====================
 * Kernel 4.14 and older
 * ===================== */
static loff_t ksu_file_proxy_remap_file_range(struct file *file_in, loff_t pos_in,
                                              struct file *file_out, loff_t pos_out,
                                              loff_t len, unsigned int remap_flags)
{
    struct ksu_file_proxy *data = file_in->private_data;
    struct file *orig = data->orig;

    /* 4.14: Hanya copy_file_range tersedia */
    if (orig->f_op->copy_file_range)
        return orig->f_op->copy_file_range(orig, pos_in, file_out, pos_out,
                                           len, 0);

    return -EINVAL;
}

#elif LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)

/* ===================================
 * Kernel 4.15 – 4.18: remap_file_range
 * =================================== */
static loff_t ksu_file_proxy_remap_file_range(struct file *file_in, loff_t pos_in,
                                              struct file *file_out, loff_t pos_out,
                                              loff_t len, unsigned int remap_flags)
{
    struct ksu_file_proxy *data = file_in->private_data;
    struct file *orig = data->orig;

    if (orig->f_op->remap_file_range)
        return orig->f_op->remap_file_range(orig, pos_in, file_out, pos_out,
                                            len, remap_flags);

    if (orig->f_op->copy_file_range)
        return orig->f_op->copy_file_range(orig, pos_in, file_out, pos_out,
                                           len, remap_flags);

    return -EINVAL;
}

#else

/* ==============================
 * Kernel 4.19+ (VFS API modern)
 * ============================== */
static int ksu_file_proxy_clone_file_range(struct file *file_in, loff_t pos_in,
                                           struct file *file_out, loff_t pos_out,
                                           u64 len)
{
    struct ksu_file_proxy *data = file_in->private_data;
    struct file *orig = data->orig;

    if (orig->f_op->clone_file_range)
        return orig->f_op->clone_file_range(orig, pos_in, file_out, pos_out, len);

    return -EINVAL;
}

static ssize_t ksu_file_proxy_dedupe_file_range(struct file *src_file, u64 loff,
                                                u64 len, struct file *dst_file, u64 dst_loff)
{
    struct ksu_file_proxy *data = src_file->private_data;
    struct file *orig = data->orig;

    if (orig->f_op->dedupe_file_range)
        return orig->f_op->dedupe_file_range(orig, loff, len, dst_file, dst_loff);

    return -EINVAL;
}

#endif


/* --- Restore missing KernelSU symbols --- */
struct ksu_file_proxy *ksu_create_file_proxy(struct file *fp)
{
    struct ksu_file_proxy *proxy;

    proxy = kzalloc(sizeof(*proxy), GFP_KERNEL);
    if (!proxy)
        return NULL;

    proxy->orig = fp;
    pr_info("KernelSU: stub ksu_create_file_proxy called for %p\n", fp);
    return proxy;
}

void ksu_delete_file_proxy(struct ksu_file_proxy *data)
{
    if (!data)
        return;

    pr_info("KernelSU: stub ksu_delete_file_proxy called for %p\n", data->orig);
    kfree(data);
}
