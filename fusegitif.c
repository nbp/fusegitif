#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define FUSE_USE_VERSION 25

#include <fuse.h>
#include <fuse_opt.h>

#include "gitstat.h"

git_repository *fg_repository();


static int
fg_getattr(const char *path, struct stat *stbuf)
{
	printf("fg_getattr is called\n");
	git_repository *repo = fg_repository();

	fg_stats *file = NULL;
	fg_file_byrepo(&file, repo, path);
	if (!file)
		return -ENOENT;

	memcpy(stbuf, fg_file_stat(file), sizeof(struct stat));

	// Copy current user info, should get this out of the the stat of the
	// repository instead of fuse_get_context.
	struct fuse_context* context = fuse_get_context();
	stbuf->st_uid = context->uid;
	stbuf->st_gid = context->gid;

	fg_stats_free(file);
	return 0;
}

struct readdir_payload
{
	// Fuse directory buffer.
	void *buf;

	// Fuse callback used for filling the buffer with directory entries.
	fuse_fill_dir_t filler;
};

static int
fg_readdir_cb(const fg_stats *dir, git_repository *repo, const char *relName, void *payload)
{
	struct readdir_payload *rd_payload = (struct readdir_payload *) payload;
	struct stat *st = NULL; // Not needed, but will cause more lookup.
	int offset = 0; // Offset of the current entry ?!
	return rd_payload->filler(rd_payload->buf, relName, st, offset);
}

static int
fg_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
						off_t offset, struct fuse_file_info *fi)
{
	printf("fg_readdir is called\n");
	(void) offset; // offset of this entry.
	(void) fi; // ??

	git_repository *repo = fg_repository();

	fg_stats *file = NULL;
	fg_file_byrepo(&file, repo, path);
	if (!file)
		return -ENOENT;

	if (!S_ISDIR(fg_file_stat(file)->st_mode)) {
		fg_stats_free(file);
		return -ENOENT;
	}

	struct readdir_payload rd_payload = {
		.buf = buf,
		.filler = filler
	};
	fg_file_list(file, repo, &fg_readdir_cb, &rd_payload);
	fg_stats_free(file);
	return 0;
}

static int
fg_open(const char *path, struct fuse_file_info *fi)
{
	printf("fg_open is called\n");
	git_repository *repo = fg_repository();

	fg_stats *file = NULL;
	fg_file_byrepo(&file, repo, path);
	if (!file) {
		printf("file not found\n");
		return -ENOENT;
	}

	const struct stat *st = fg_file_stat(file);
	printf("fg_open: stbuf:\n\tmode %o\n\tnlink %d\n\tsize %d\n",
			st->st_mode,
			st->st_nlink,
			st->st_size);

	// :TODO: Check if this match file permissions instead. Currently this is fine
	// as all files are marked as readonly.
	if((fi->flags & 3) != O_RDONLY) {
		fg_stats_free(file);
		return -EACCES;
	}

	fg_stats_free(file);
	return 0;
}

static int
fg_read(const char *path, char *buf, size_t size, off_t offset,
				struct fuse_file_info *fi)
{
	printf("fg_read is called\n");
	(void) fi;

	git_repository *repo = fg_repository();

	fg_stats *file = NULL;
	fg_file_byrepo(&file, repo, path);
	if (!file) {
		printf("file not found\n");
		return -ENOENT;
	}

	const struct stat *st = fg_file_stat(file);
	printf("fg_read: stbuf:\n\tmode %o\n\tnlink %d\n\tsize %d\n",
			st->st_mode,
			st->st_nlink,
			st->st_size);

	if (offset < st->st_size) {
		printf("file not found\n");
		// Check if the requested size goes beyong the file size.
		if (offset + size > st->st_size)
			size = st->st_size - offset;

		// Copy the content out of the git repository.
		if (fg_file_cpy(buf, repo, file, offset, size)) {
			fg_stats_free(file);
			return -ENOENT;
		}
	} else {
		// Read an offset which is not contained in the file.
		size = 0;
	}

	fg_stats_free(file);
	return size;
}


// Store global parameters which are all initialized in the main function and
// clean-up in the main function and read-only for all others. Other function
// should use the accessors (which should later be moved to another file).
struct fg_options {
	// Store the repository name.
	char *repoName;

	// Keep a global instance of the repository open for the duration of the
	// mount-point.
	git_repository *repo;
};

struct fg_options options;

git_repository *
fg_repository()
{
	return options.repo;
}

// macro to define options
#define FG_CLI_KEY(t, p, v) { t, offsetof(struct fg_options, p), v }

static struct fuse_opt fg_cli[] =
{
	// Register the repository name.
	FG_CLI_KEY("--repository=%s", repoName, 0),
	FG_CLI_KEY("-r %s", repoName, 0),

	// No more arguments.
	FUSE_OPT_END
};

static struct fuse_operations fg_oper = {
	.getattr	= fg_getattr,
	.readdir	= fg_readdir,
	.open	= fg_open,
	.read	= fg_read,
};

int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	/* clear structure that holds our options */
	memset(&options, 0, sizeof(struct fg_options));
	if (fuse_opt_parse(&args, &options, fg_cli, NULL) == -1) {
		// Error parsing options
		return -1;
	}

	if (options.repoName == NULL) {
		// Expect a repository name.
		fuse_opt_free_args(&args);
		return -2;
	}

	if (git_repository_open(&options.repo, options.repoName)) {
		// Cannot open the repository.
		fuse_opt_free_args(&args);
		return -3;
	}

	int ret = fuse_main(args.argc, args.argv, &fg_oper);

//	if (ret) printf("\n");

	// Clean-up
	git_repository_free(options.repo);
	// The name has been allocated by fuse.
	free(options.repoName);
	fuse_opt_free_args(&args);

	return ret;
}

