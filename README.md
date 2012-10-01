fusegitif - Mount a git repository.
======================

fusegitif mount branches of a Git repository. This can be useful for browsing,
comparing, or even building sources.

Status
======================
Version 0.0
	Not working yet.

Done for version 0.1:
* Query file properties with libgit2.

For version 0.1:
* Add a dummy fuse implementation with no cache.
* Add Automatic tests fusegitif file-system queries without fuse.

For later:
* Cache stats of files in non-changing branches.
* Recover creation / modification times of files.
* Emulate access time.
* Add remote branches.
* Branches as symbolic links to branches or commits.

Dependencies
======================
* libfuse <http://fuse.sourceforge.net/>
* libgit2 <https://github.com/libgit2/libgit2>

Similar Project
======================
* git-fuse-perl <https://github.com/mfontani/git-fuse-perl>
		This implementation is extremelly nice, but it does not provide a good
		approach for making incremental builds of branches because timestamps
		are not maintained across filesystem visits.
		In addition, it relies on git commands execution which are causing
		additional slow-down for visiting branches and reading files.

