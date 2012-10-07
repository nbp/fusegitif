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
const char *fg_file_path(const fg_stats *file);

// Non-zero if the file is the root of commit tree targeted by a branch.
int fg_file_is_branch_root(const fg_stats *file);

// Non-zero if this file can be lookup in the git repository.
int fg_file_has_oid(const fg_stats *file);

// Git object identifier corresponding to this file.
//
// The lifetime of this object identifer is bounded to the lifetime of the file.
// This Object identifier does not garantee that the object will still be alive
// in the Git database.
const git_oid *fg_file_oid(const fg_stats *file);

// Stat of the file in the emulated filesystem.
//
// The lifetime of this pointer is bounded to the lifetime of the file.
const struct stat *fg_file_stat(const fg_stats *file);

// Read a file content from an offset and for a specific size.
int fg_file_cpy(void *dest, git_repository *repo, const fg_stats *file, size_t fileOffset, size_t size);

// Callback used by fg_file_list.
//
// @param dir  Parent directory used in fg_file_list.
// @param relName  Relative name of the file relative to the directory.
// @param payload  Untyped data transfered from fg_file_list.
typedef int (*fg_list)(const fg_stats *dir, git_repository *repo, const char *relName, void *payload);

// List files stored in a directory.  If the file is not a tree or a branch
// prefix, then an error code is returned.
//
// @param file  directory which content is listed.
// @param callback  Function called for each file contained in this directory.
// @param payload  Untyped data transfered to the callback.
int fg_file_list(const fg_stats *file, git_repository *repo, fg_list callback, void *payload);

