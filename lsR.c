#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <sys/stat.h>

#include <git2.h>

int print_oid(const char *prefix, const git_oid *oid) {
  char oidstr[GIT_OID_HEXSZ+1];
  return printf("%s oid: %s\n", prefix, git_oid_tostr(oidstr, -1, oid));
}

struct fg_stats {
  // Branch name followed by the object name.
  char *path;
  // Name of the object inside the branch.
  char *object;

  // Collect informations from commits to determine the last modification date
  // of the file.
  struct stat stbuf;

  // Object Identifier
  git_oid oid;
};

typedef struct fg_stats fg_stats;

void
fg_stats_free(fg_stats *stats)
{
  if (!stats)
    return;
  free(stats->path);
  free(stats);
}

int
fg_dir_count_subtree(const char *root, const git_tree_entry *entry, void *payload)
{
  // :TODO: Handle submodules.
  if (git_tree_entry_filemode(entry) == GIT_FILEMODE_TREE) {
    int *nlink = (int*) payload;
    *nlink += 1;
  }
  // Skip deep traversal
  return 1;
}

int
fg_file_byentry(fg_stats **out, git_repository *repo, git_tree_entry *entry)
{
  const git_oid *oid = git_tree_entry_id(entry);
  git_otype type = git_tree_entry_type(entry);
  git_filemode_t mode = git_tree_entry_filemode(entry);

  mode_t st_mode = 0;

  // Recover the number of hard links of directories.
  // :TODO: Handle submodules.
  int nlink = 1;
  if (mode == GIT_FILEMODE_TREE) {
    st_mode = S_IFDIR | 0555;

    git_tree *sub = NULL;
    if (git_tree_lookup(&sub, repo, oid))
      return -9;
    // A directory contains '.' which refer to it-self.
    nlink += 1;
    // Add 1 for each sub-directories, which refer to its parent with '..'
    if (git_tree_walk(sub, &fg_dir_count_subtree, GIT_TREEWALK_PRE, &nlink) < 0) {
      git_tree_free(sub);
      return -10;
    }
    git_tree_free(sub);
  }

  // Recover the file size of any plain file.
  size_t size = 0;
  if (mode == GIT_FILEMODE_BLOB ||
      mode == GIT_FILEMODE_BLOB_EXECUTABLE ||
      mode == GIT_FILEMODE_LINK) {
    st_mode =
      (mode == GIT_FILEMODE_LINK ? S_IFLNK : S_IFREG) |
      (mode == GIT_FILEMODE_BLOB_EXECUTABLE ? 0555 : 0444);

    git_blob *blob = NULL;
    if (git_blob_lookup(&blob, repo, oid))
      return -11;
    size = git_blob_rawsize(blob);
    git_blob_free(blob);
  }

  // Found !!!
  // reset all the field and fill in what we found.
  fg_stats *result = calloc(1, sizeof(fg_stats));

  result->path = NULL;
  result->object = NULL;

  // Register collected data.
  result->stbuf.st_mode = st_mode;
  result->stbuf.st_nlink = nlink;
  result->stbuf.st_size = size;

  // It is easier for us if tools can load files in one request instead of
  // multiple, such that we don't do a lookup again for serving the same file,
  // so let us lie to the system and inform it that this file is only one block.
  result->stbuf.st_blksize = size;
  result->stbuf.st_blocks = 1;

  git_oid_cpy(&result->oid, oid);

  *out = result;
  return 0;
}

int
fg_file_byroot(fg_stats **out, git_repository *repo, git_commit *commit)
{
  const git_oid *oid = git_commit_tree_oid(commit);

  // Recover the number of hard links of directories.
  // :TODO: Handle submodules.
  // :TODO: This code is shared with the previous function, factor it out.
  int nlink = 1;
  mode_t st_mode = S_IFDIR | 0555;

  git_tree *sub = NULL;
  if (git_tree_lookup(&sub, repo, oid))
    return -9;
  // A directory contains '.' which refer to it-self.
  nlink += 1;
  // Add 1 for each sub-directories, which refer to its parent with '..'
  if (git_tree_walk(sub, &fg_dir_count_subtree, GIT_TREEWALK_PRE, &nlink) < 0) {
    git_tree_free(sub);
    return -10;
  }
  git_tree_free(sub);

  // Recover the file size of any plain file.
  size_t size = 0;

  // reset all the field and fill in what we found.
  fg_stats *result = calloc(1, sizeof(fg_stats));

  result->path = NULL;
  result->object = NULL;

  // Register collected data.
  result->stbuf.st_mode = st_mode;
  result->stbuf.st_nlink = nlink;

  git_oid_cpy(&result->oid, oid);

  *out = result;
  return 0;
}

int
fg_file_bytree(fg_stats **out, git_repository *repo, git_tree *tree, const char *path)
{
  git_tree_entry *entry;
  if (git_tree_entry_bypath(&entry, tree, path))
    return -8;

  int exit = fg_file_byentry(out, repo, entry);
  git_tree_entry_free(entry);
  return exit;
}

int
fg_file_bycommit(fg_stats **out, git_repository *repo, git_commit *commit, const char *path)
{
  if (path[0] == '\0')
    return fg_file_byroot(out, repo, commit);

  git_tree *commitTree = NULL;
  if (git_commit_tree(&commitTree, commit))
    return -7;

  int exit = fg_file_bytree(out, repo, commitTree, path);
  git_tree_free(commitTree);
  return exit;
}

int
fg_file_bybranch(fg_stats **out, git_repository *repo, git_reference *ref, const char *path)
{
  // Get the Object Identifier of the commit.
  const git_oid *oid = git_reference_oid(ref);
  if (!oid)
    return -5;

  print_oid("commit", oid);

  git_commit *commit = NULL;
  if (git_commit_lookup(&commit, repo, oid))
    return -6;

  int exit = fg_file_bycommit(out, repo, commit, path);

  if (*out) {
    fg_stats *result = *out;
    time_t last = git_commit_time(commit);
    result->stbuf.st_atime = last;
    result->stbuf.st_mtime = last;
    result->stbuf.st_ctime = last;
  }

  git_commit_free(commit);
  return exit;
}

int
fg_file_bysymbref(fg_stats **out, git_repository *repo, git_reference *ref, const char *path)
{
  // Resolve symbolic branches.
  git_reference *direct = NULL;
  if (git_reference_resolve(&direct, ref) != 0)
    return -4;

  int exit = fg_file_bybranch(out, repo, direct, path);
  git_reference_free(direct);
  return exit;
}

struct fg_match_args
{
  const char *prefix;
  size_t len;
  int nlink;
};

int
fg_match_branch_prefix(const char *branch, git_branch_t type, void *payload)
{
  struct fg_match_args *args = (struct fg_match_args *) payload;
  if (strncmp(branch, args->prefix, args->len) == 0) {
    // :FIXME: This is wrong for branches with 2 or more slash in their names.
    args->nlink += 1;
  }
  return 0;
}

int
fg_file_byprefix(fg_stats **out, git_repository *repo, char *path)
{
  size_t len = strlen(path);
  struct fg_match_args args;

  // Add a trailing slash to match directory-like prefixes. This is safe, see
  // allocation in fg_file_repo.
  path[len] = '/';
  args.prefix = path;
  args.len = len + 1;
  args.nlink = 1; // to account for '.'.
  int error = git_branch_foreach(repo, GIT_BRANCH_LOCAL, &fg_match_branch_prefix, &args);
  path[len] = '\0';

  if (error != 0)
    return -2;

  // No such prefix.
  if (args.nlink == 1)
    return -1;

  // reset all the field and fill in what we found.
  fg_stats *result = calloc(1, sizeof(fg_stats));

  result->path = NULL;
  result->object = NULL;
  result->stbuf.st_mode = S_IFDIR | 0555;
  result->stbuf.st_nlink = args.nlink;

  *out = result;
  return 0;
}

int
fg_file_byrepo(fg_stats **out, git_repository *repo, const char *path)
{
  size_t len = strlen(path);
  char *branch = NULL;
  char *object = NULL;

  // Allocate extra space, such as we can append an extra slash and know if this
  // branch name is branch prefix.
  branch = malloc(len + 2);
  if (!branch)
    return -3;
  strcpy(branch, path);
  branch[len + 1] = '\0';

  // Remove the trailing slash
  if (branch[len - 1] == '/')
    branch[len - 1] = '\0';

  object = branch;
  git_reference *symb = NULL;
  // Search if we have a branch name
  while (object = strchr(object, '/')) {
    *object++ = '\0';
    printf("branch %s , object %s ?\n", branch, object);
    if (git_branch_lookup(&symb, repo, branch, GIT_BRANCH_LOCAL) == 0)
      break;
    object[-1] = '/';
  }

  if (object == NULL) {
    if (git_branch_lookup(&symb, repo, branch, GIT_BRANCH_LOCAL) == 0) {
      // We have hit the top level of a repository.
      printf("branch %s\n", branch);
      object = branch + len;
      assert(symb != NULL);
    } else {
      // Is this a repository prefix ? append a '/' and check if this is
      // matching any branch name prefix.
      printf("prefix %s ?\n", branch);
      assert(symb == NULL);
    }
  } else {
    printf("branch %s , object %s\n", branch, object);
    assert(symb != NULL);
  }

  int exit = 0;
  if (symb) {
    assert(object != NULL);
    exit = fg_file_bysymbref(out, repo, symb, object);
  } else {
    assert(object == NULL);
    exit = fg_file_byprefix(out, repo, branch);
  }

  if (*out) {
    fg_stats *result = *out;
    result->path = branch;
    result->object = object;
  } else {
    free(branch);
  }

  git_reference_free(symb);
  return exit;
}

int
main(int argc, char **argv)
{
  if (argc < 2)
    return -1;

  git_repository *repo;
  printf("Open repository %s\n", argv[1]);
  if (git_repository_open(&repo, argv[1]))
    return -2;

  for (int i = 2; i < argc; i++)
  {
    fg_stats *st = NULL;
    int found = fg_file_byrepo(&st, repo, argv[i]);
    if (st && st->object) {
      char oidstr[GIT_OID_HEXSZ+1];
      printf("%s: Object %s [%s]\n",
          st->path, st->object,
          git_oid_tostr(oidstr, -1, &st->oid));
    } else if (st) {
      printf("%s: Path to a branch.\n", argv[i]);
    } else {
      printf("%s: not found [error %d]\n", argv[i], found);
    }
    if (st) {
      printf("stbuf:\n\tmode %o\n\tnlink %d\n\tsize %d\n",
          st->stbuf.st_mode,
          st->stbuf.st_nlink,
          st->stbuf.st_size);
    }
    fg_stats_free(st);
  }

  git_repository_free(repo);
  return 0;
}
