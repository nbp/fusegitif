#include <sys/stat.h>
#include <git2.h>

struct fg_stats;
typedef struct fg_stats fg_stats;

// Free file stats.
//
// @param stats Stats to free.
void fg_stats_free(fg_stats *stats);

// Find a file located at <branch>/<path> inside a repository.
//
// Allocate the stats of the current file and return 0 in case of success,
// otherwise return a negative error code. Resources returned in *out must be
// freed with fg_stats_free. The lifetime is not bounded except if a GC remove
// the object from the repository. If a GC occur, the next lookup using the
// git_oid will fail.
//
// @param out Pointer where to store the stat of the file.
//
// @param path Path of the file in the emulated file system hierachy.
//
// @return 0 or an error code.
int fg_file_byrepo(fg_stats **out, git_repository *repo, const char *path);

// Path of the file in the emulated filesystem.
const char *fg_file_path(fg_stats *file);

// Non-zero if the file is the root of commit tree targeted by a branch.
int fg_file_is_branch_root(fg_stats *file);

// Non-zero if this file can be lookup in the git repository.
int fg_file_has_oid(fg_stats *file);

// Git object identifier corresponding to this file.
//
// The lifetime of this object identifer is bounded to the lifetime of the file.
// This Object identifier does not garantee that the object will still be alive
// in the Git database.
const git_oid *fg_file_oid(fg_stats *file);

// Stat of the file in the emulated filesystem.
//
// The lifetime of this pointer is bounded to the lifetime of the file.
struct stat *fg_file_stat(fg_stats *file);

