#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "gitstat.h"

int print_oid(const char *prefix, const git_oid *oid) {
  char oidstr[GIT_OID_HEXSZ+1];
  return printf("%s oid: %s\n", prefix, git_oid_tostr(oidstr, -1, oid));
}

int listDir(const fg_stats *dir, git_repository *repo, const char *relName, void *payload){
  printf("\t%s\n", relName);
  return 0;
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
    fg_stats *file = NULL;
    int found = fg_file_byrepo(&file, repo, argv[i]);
    if (file) {
      struct stat *st = fg_file_stat(file);
      char oidstr[GIT_OID_HEXSZ+1];
      printf("%s [%s]\n",
          fg_file_path(file),
          (fg_file_has_oid(file) ? git_oid_tostr(oidstr, -1, fg_file_oid(file)) : "branch-prefix"));
      printf("stbuf:\n\tmode %o\n\tnlink %d\n\tsize %d\n",
          st->st_mode,
          st->st_nlink,
          st->st_size);

      if (S_ISDIR(st->st_mode)) {
        printf("-> Is a directory containing:\n");
        fg_file_list(file, repo, &listDir, NULL);
      }
    } else {
      printf("%s: not found [error %d]\n", argv[i], found);
    }
    fg_stats_free(file);
  }

  git_repository_free(repo);
  return 0;
}
