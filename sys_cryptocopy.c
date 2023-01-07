// SPDX-License-Identifier: GPL-2.0
#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/module.h>
// for struct filename
#include <linux/fs.h>
// for user_path_at_empty
// for do_renameat2
#include <linux/namei.h>
// for current->fs
#include <linux/fs_struct.h>
// for get_fs/set_fs
#include <linux/uaccess.h>
// for kmalloc
#include <linux/slab.h>
// for get_random_bytes
#include <linux/random.h>
// for crypto_cipher
#include <linux/crypto.h>
// for crypto API functions
#include <crypto/skcipher.h>
// for sg_init_one
#include <linux/scatterlist.h>
// for __GFP_ZERO
#include <linux/gfp.h>
// for crypto_alloc_shash
#include <crypto/hash.h>
#include "sys_cryptocopy.h"

#define EXTRA_CREDIT
#define TMPLEN 1
#define MAX_TMP_ATTEMPTS 50

asmlinkage extern long (*sysptr)(char *infile, char *outfile, char *keybuf,
				 size_t keylen, int flags);

struct file *tmpfile(const char *orig, umode_t mode, struct filename **fnp)
{
	struct filename *fn = ERR_PTR(-1);
	struct file *tmp = ERR_PTR(-1);
	size_t len;
	char *name = ERR_PTR(-1);
	int i, attempt;
	unsigned char c;
	char *tmpdir = "/tmp/";

	len = strlen(orig);
	if (len + TMPLEN + 1 > PATH_MAX) {
		orig = tmpdir;
		len = strlen(orig);
	}
	name = kmalloc(len + TMPLEN + 1, GFP_KERNEL);
	if (!name)
		return ERR_PTR(-ENOMEM);
	for (i = 0; i < len; i++)
		name[i] = orig[i];
	name[len + TMPLEN] = '\0';
	for (attempt = 0; attempt < MAX_TMP_ATTEMPTS; attempt++) {
		get_random_bytes(&name[len], TMPLEN);
		// convert to [a-zA-Z]
		for (i = 0; i < TMPLEN; i++) {
			c = name[len + i];
			c %= 52;
			if (c > 25)
				c += 6;
			c += 'A';
			name[len + i] = c;
		}
		fn = getname_kernel(name);
		// must be ENOMEM since name can't be too long
		if (IS_ERR(fn)) {
			tmp = (void *)fn;
			goto out;
		}
		tmp = file_open_name(fn, O_CREAT | O_RDWR | O_EXCL, mode);
		if (!IS_ERR(tmp))
			break;
		putname(fn);
	}
out:
	kfree(name);
	if (!IS_ERR(tmp))
		*fnp = fn;
	return tmp;
}

// 0-3			digest
// 4-19			iv
int preamble(struct file *f, char *keybuf, size_t keylen, u64 *iv,
	     unsigned int ivsize, int flags)
{
	int ret = 0;
	char *hashdata = kmalloc(keylen + ivsize, GFP_KERNEL);
	u32 digest;
	u32 check_digest;
	struct crypto_shash *tfm = ERR_PTR(-1);
	SHASH_DESC_ON_STACK(desc, tfm);
	ssize_t bytes_rw;
	mm_segment_t oldfs;

	if (!hashdata)
		return -ENOMEM;
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	tfm = crypto_alloc_shash("crc32", 0, 0);
	if (IS_ERR(tfm)) {
		ret = PTR_ERR(tfm);
		goto out;
	}
	desc->tfm = tfm;
	memcpy(hashdata, keybuf, keylen);
	// why would a digest need a key...?
	//ret = crypto_shash_setkey(tfm, hash, keylen + ivsize);
	//if (ret < 0) goto out;
	// should store hash of key concat IV
	// so files encrypted with same key have diff hash
	if (flags & FLAG_ENCRYPT) {
		// copy iv to hash
		memcpy(hashdata + keylen, iv, ivsize);
		ret = crypto_shash_digest(desc, hashdata, keylen + ivsize,
					  (void *)&digest);
		if (ret < 0)
			goto out;
		// write hash of (key and iv) to f
		bytes_rw =
			vfs_write(f, (void *)&digest, sizeof(digest),
				  f->f_mode & FMODE_STREAM ? NULL : &f->f_pos);
		if (bytes_rw < 0) {
			ret = bytes_rw;
			goto out;
		}
		if (bytes_rw < sizeof(digest)) {
			ret = -EIO;
			goto out;
		}
		// write iv to f
		bytes_rw =
			vfs_write(f, (void *)iv, ivsize,
				  f->f_mode & FMODE_STREAM ? NULL : &f->f_pos);
		if (bytes_rw < 0) {
			ret = bytes_rw;
			goto out;
		}
		if (bytes_rw < ivsize)
			ret = -EIO;
	} else if (flags & FLAG_DECRYPT) {
		// validate hash of key and set iv
		// read iv and copy to hash
		bytes_rw =
			vfs_read(f, (void *)&digest, sizeof(digest),
				 f->f_mode & FMODE_STREAM ? NULL : &f->f_pos);
		if (bytes_rw < 0) {
			ret = bytes_rw;
			goto out;
		}
		if (bytes_rw < sizeof(digest)) {
			// file too small
			ret = -EINVAL;
			goto out;
		}
		bytes_rw =
			vfs_read(f, (void *)iv, ivsize,
				 f->f_mode & FMODE_STREAM ? NULL : &f->f_pos);
		if (bytes_rw < 0) {
			ret = bytes_rw;
			goto out;
		}
		if (bytes_rw < ivsize) {
			ret = -EINVAL;
			goto out;
		}
		memcpy(hashdata + keylen, iv, ivsize);
		ret = crypto_shash_digest(desc, hashdata, keylen + ivsize,
					  (void *)&check_digest);
		if (ret < 0)
			goto out;
		if (check_digest != digest)
			ret = -EACCES;
	}
out:
	set_fs(oldfs);
	shash_desc_zero(desc);
	kfree(hashdata);
	if (!IS_ERR(tfm))
		crypto_free_shash(tfm);
	return ret;
}

int copy(struct file *in, struct file *out, char *keybuf, size_t keylen,
	 int flags)
{
	int ret = 0;
	ssize_t bytes_read;
	ssize_t bytes_wrote = PAGE_SIZE;
	void *buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	mm_segment_t oldfs;
	struct crypto_sync_skcipher *tfm = ERR_PTR(-1);
	struct skcipher_request *req = ERR_PTR(-1);
	struct scatterlist sg;
	unsigned int ivsize;
	int crypt = flags & (FLAG_DECRYPT | FLAG_ENCRYPT);
	u64 *iv = NULL;

	if (!buf)
		return -ENOMEM;
	if (crypt) {
		tfm = crypto_alloc_sync_skcipher("ctr(aes)", 0, 0);
		if (IS_ERR(tfm)) {
			ret = PTR_ERR(tfm);
			goto out;
		}
		ret = crypto_sync_skcipher_setkey(tfm, keybuf, keylen);
		if (ret < 0)
			goto out;
		req = skcipher_request_alloc(&tfm->base, GFP_KERNEL);
		if (!req) {
			ret = -ENOMEM;
			goto out;
		}
		ivsize = crypto_sync_skcipher_ivsize(tfm);
		iv = kmalloc(ivsize, GFP_KERNEL | __GFP_ZERO);
		if (!iv) {
			ret = -ENOMEM;
			goto out;
		}
#ifdef EXTRA_CREDIT
		iv[ivsize/8 - 1] = in->f_inode->i_ino;
#endif
		ret = preamble(flags & FLAG_ENCRYPT ? out : in, keybuf, keylen,
			       iv, ivsize, flags);
		if (ret < 0)
			goto out;
	}
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	while (bytes_wrote == PAGE_SIZE) {
		bytes_read =
			vfs_read(in, buf, PAGE_SIZE,
				 in->f_mode & FMODE_STREAM ? NULL : &in->f_pos);
		if (bytes_read < 0) {
			ret = bytes_read;
			break;
		}
		if (crypt) {
			sg_init_one(&sg, buf, bytes_read);
			skcipher_request_set_crypt(req, &sg, &sg, bytes_read,
						   iv);
			if (flags & FLAG_ENCRYPT)
				ret = crypto_skcipher_encrypt(req);
			else
				ret = crypto_skcipher_decrypt(req);
			if (ret < 0)
				break;
#ifdef EXTRA_CREDIT
			iv[0]++;
#endif
		}
		bytes_wrote = vfs_write(out, buf, bytes_read, out->f_mode
								& FMODE_STREAM
								? NULL :
								&out->f_pos);
		if (bytes_wrote < 0) {
			ret = bytes_wrote;
			break;
		} else if (bytes_wrote < bytes_read) {
			ret = -EIO;
			break;
		}
	}
	set_fs(oldfs);
out:
	kfree(buf);
	if (!iv)
		kfree(iv);
	if (!IS_ERR(tfm))
		crypto_free_sync_skcipher(tfm);
	if (!IS_ERR(req))
		skcipher_request_free(req);
	return ret;
}

asmlinkage long cryptocopy(char *infile, char *outfile, char *keybuf,
			   size_t keylen, int flags)
{
	struct filename *inname = ERR_PTR(-1);
	struct filename *outname = ERR_PTR(-1);
	struct filename *tmpname = ERR_PTR(-1);
	struct file *in = ERR_PTR(-1);
	struct file *out = ERR_PTR(-1);
	struct file *tmp = ERR_PTR(-1);
	char *key = NULL;
	struct path path;
	struct kstat stat;
	mm_segment_t oldfs;
	int error;
	int ret = 0;
	int mode = flags & (FLAG_ENCRYPT | FLAG_DECRYPT | FLAG_COPY);

	if (mode != FLAG_ENCRYPT && mode != FLAG_DECRYPT && mode != FLAG_COPY)
		return -EINVAL;
	// key must be length 16, 24, or 32 for ctr(aes)
	if (mode != FLAG_COPY && keylen != 32 && keylen != 24 && keylen != 16)
		return -EINVAL;
	inname = getname(infile);
	if (IS_ERR(inname)) {
		ret = PTR_ERR(inname);
		goto out;
	}
	outname = getname(outfile);
	if (IS_ERR(outname)) {
		ret = PTR_ERR(outname);
		goto out;
	}
	if (flags & (FLAG_DECRYPT | FLAG_ENCRYPT)) {
		key = kmalloc(keylen + 1, GFP_KERNEL);
		if (!key) {
			ret = -ENOMEM;
			goto out;
		}
		// returns number of bytes that couldn't by copied
		error = raw_copy_from_user(key, keybuf, keylen);
		if (error != 0) {
			ret = -EFAULT;
			goto out;
		}
	}
	// TODO check behavior for irregular files
	in = file_open_name(inname, O_RDONLY, 0);
	if (IS_ERR(in)) {
		ret = PTR_ERR(in);
		goto out;
	}
	// get protection mode for in file
	error = user_path_at_empty(AT_FDCWD, infile,
				   LOOKUP_FOLLOW | LOOKUP_AUTOMOUNT, &path,
				   NULL);
	if (error) {
		ret = error;
		goto out;
	}
	error = vfs_getattr(&path, &stat, STATX_MODE, AT_NO_AUTOMOUNT);
	if (error) {
		ret = error;
		goto out;
	}
	if (!S_ISREG(stat.mode)) {
		if (S_ISDIR(stat.mode))
			ret = -EISDIR;
		else
			ret = -EINVAL;
		goto out;
	}
	out = file_open_name(outname, O_CREAT | O_WRONLY | O_EXCL,
			     stat.mode & ~(current->fs->umask));
	if (IS_ERR(out)) {
		if (PTR_ERR(out) != -EEXIST) {
			ret = PTR_ERR(out);
			goto out;
		}
		// we will need to use tmpfile, but first check outfile
		// is still valid
		// so open without excl to check permissions and make
		// sure different from in
		out = file_open_name(outname, O_CREAT | O_WRONLY,
				     stat.mode & ~(current->fs->umask));
		if (IS_ERR(out)) {
			ret = PTR_ERR(out);
			goto out;
		}
		if (out->f_inode && !S_ISREG(out->f_inode->i_mode)) {
			ret = -EINVAL;
			goto out;
		}
		if (in->f_inode && out->f_inode &&
		    in->f_inode->i_ino == out->f_inode->i_ino) {
			ret = -EINVAL;
			goto out;
		}
		// tmpname should only need putname if !IS_ERR(tmp)
		// & with permissions of existing outfile
		tmp = tmpfile(outname->name,
			      stat.mode & ~(current->fs->umask) &
				      out->f_inode->i_mode,
			      &tmpname);
		filp_close(out, 0);
		out = ERR_PTR(-1);
		if (IS_ERR(tmp)) {
			if (PTR_ERR(tmp) == -EEXIST)
				pr_info("unable to create unique tmp file\n");
			ret = PTR_ERR(tmp);
			goto out;
		}
		ret = copy(in, tmp, key, keylen, flags);
		if (ret < 0)
			goto out;
		oldfs = get_fs();
		set_fs(KERNEL_DS);
		ret = do_renameat2(AT_FDCWD, tmpname->name, AT_FDCWD,
				   outname->name, 0);
		if (ret < 0)
			pr_info("failed to rename tmpfile %s to %s",
				tmpname->name, outname->name);
		set_fs(oldfs);
	} else {
		// successfully opened new output file (no need to use tmp file)
		// check they are not the same file
		if (in->f_inode && out->f_inode &&
		    in->f_inode->i_ino == out->f_inode->i_ino) {
			ret = -EINVAL;
			goto out;
		}
		ret = copy(in, out, key, keylen, flags);
		filp_close(out, 0);
		out = ERR_PTR(-1);
		if (ret < 0)
			do_unlinkat(AT_FDCWD, outname);
	}
out:
	if (!IS_ERR(in))
		filp_close(in, 0);
	if (!IS_ERR(out))
		filp_close(out, 0);
	if (!IS_ERR(tmp))
		filp_close(tmp, 0);
	if (!IS_ERR(inname))
		putname(inname);
	// we might have unlinked outname
	if (!IS_ERR(outname) && outname->refcnt > 0)
		putname(outname);
	if (!IS_ERR(tmpname)) {
		// note that do_unlinkat calls putname(tmpname) for us
		error = do_unlinkat(AT_FDCWD, tmpname);
		// tmpfile should be removed anyway by rename, so something else
		// already went wrong if error != -ENOENT
		if (error && error != -ENOENT)
			pr_info("sys_cryptocopy failed to remove tmp file\n");
	}
	// according to checkpatch kfree(NULL) is safe
	kfree(key);
	return ret;
}

static int __init init_sys_cryptocopy(void)
{
	if (!sysptr)
		sysptr = cryptocopy;
	return 0;
}

static void __exit exit_sys_cryptocopy(void)
{
	if (sysptr)
		sysptr = NULL;
}

module_init(init_sys_cryptocopy);
module_exit(exit_sys_cryptocopy);
MODULE_LICENSE("GPL");
