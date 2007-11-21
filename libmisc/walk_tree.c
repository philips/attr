/*
  File: walk_tree.c

  Copyright (C) 2007 Andreas Gruenbacher <a.gruenbacher@computer.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "walk_tree.h"

static int walk_tree_rec(const char *path, int walk_flags,
			 int (*func)(const char *, const struct stat *, int, void *),
			 void *arg, int depth)
{
	int (*xstat)(const char *, struct stat *) = lstat;
	struct stat st;
	int local_walk_flags = walk_flags, err;

	/* Default to traversing symlinks on the command line, traverse all symlinks
	 * with -L, and do not traverse symlinks with -P. (This is similar to chown.)
	 */

follow_symlink:
	if (xstat(path, &st) != 0)
		return func(path, NULL, local_walk_flags | WALK_TREE_FAILED, arg);
	if (S_ISLNK(st.st_mode)) {
		if ((local_walk_flags & WALK_TREE_PHYSICAL) ||
		    (!(local_walk_flags & WALK_TREE_LOGICAL) && depth > 1))
			return 0;
		local_walk_flags |= WALK_TREE_SYMLINK;
		xstat = stat;
		if (local_walk_flags & WALK_TREE_DEREFERENCE)
			goto follow_symlink;
	}
	err = func(path, &st, local_walk_flags, arg);
	if ((local_walk_flags & WALK_TREE_RECURSIVE) &&
	    (S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode))) {
		char path2[FILENAME_MAX];
		DIR *dir;
		struct dirent *entry;
		int err2;

		dir = opendir(path);
		if (!dir) {
			/* PATH may be a symlink to a regular file or a dead symlink
			 * which we didn't follow above.
			 */
			if (errno != ENOTDIR && errno != ENOENT)
				err += func(path, &st,
					    local_walk_flags | WALK_TREE_FAILED, arg);
			return err;
		}
		while ((entry = readdir(dir)) != NULL) {
			if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
				continue;
			err2 = snprintf(path2, sizeof(path2), "%s/%s", path,
				       entry->d_name);
			if (err2 < 0 || err2 > FILENAME_MAX) {
				errno = ENAMETOOLONG;
				err += func(path, NULL,
					    local_walk_flags | WALK_TREE_FAILED, arg);
				continue;
			}
			err += walk_tree_rec(path2, walk_flags, func, arg, depth + 1);
		}
		if (closedir(dir) != 0)
			err += func(path, &st, local_walk_flags | WALK_TREE_FAILED, arg);
	}
	return err;
}

int walk_tree(const char *path, int walk_flags,
	      int (*func)(const char *, const struct stat *, int, void *), void *arg)
{
	if (strlen(path) >= FILENAME_MAX) {
		errno = ENAMETOOLONG;
		return func(path, NULL, WALK_TREE_FAILED, arg);
	}
	return walk_tree_rec(path, walk_flags, func, arg, 1);
}
