/*
 * Copyright (C) 2001 by Andreas Gruenbacher <a.gruenbacher@computer.org>
 *
 * TODO: should this be replaced by using nftw(3)?
 */
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "walk_tree.h"

int walk_recurse = 0;
int walk_postorder = 0;
int walk_symlinks = WALK_HALF_LOGICAL;

int walk_tree(const char *path_p, int (*call)(const char *, struct stat *,
              void *), void *arg)
{
	static struct stat st;
	static int level = 0;
	int follow;
	int errors = 0;

	level++;
	if (lstat(path_p, &st) != 0)
		goto fail;

	if (S_ISLNK(st.st_mode)) {
		if (walk_symlinks == WALK_PHYSICAL)
			follow = 0;
		else {
			if (stat(path_p, &st) != 0)
				goto fail;
			follow = ((walk_symlinks == WALK_HALF_LOGICAL &&
                                   level == 1) ||
		                   walk_symlinks == WALK_FULL_LOGICAL);
		}
	} else
		follow = 1;

	if (!follow)
		goto cleanup;

	if (!walk_postorder)
		errors += call(path_p, &st, arg);

	if (walk_recurse && S_ISDIR(st.st_mode)) {
		struct dirent *dirent;
		DIR *dir;

		char *ipath_p = (char *)path_p;
		int ipath_length = strlen(ipath_p);
		if (ipath_p[ipath_length-1] != '/') {
			ipath_p = (char*)alloca(ipath_length +
			                        _POSIX_PATH_MAX + 2);
			if (ipath_p == NULL)
				goto fail;
			strcpy(ipath_p, path_p);
			strcpy(ipath_p + ipath_length, "/");
			ipath_length++;
		}

		dir = opendir(path_p);
		if (dir == NULL)
			goto fail;
		while ((dirent = readdir(dir)) != NULL) {
			if (!strcmp(dirent->d_name, ".") ||
			    !strcmp(dirent->d_name, ".."))
				continue;
			strncpy(ipath_p + ipath_length,
			        dirent->d_name, _POSIX_PATH_MAX);
			ipath_p[ipath_length + _POSIX_PATH_MAX] = '\0';
			walk_tree(ipath_p, call, arg);  /* recurse */
		}
		ipath_p[ipath_length] = '\0';
		closedir(dir);
	}

	if (walk_postorder)
		errors += call(path_p, &st, arg);

cleanup:
	level--;
	return errors;

fail:
	fprintf(stderr, "%s: %s: %s\n", progname, path_p, strerror(errno));
	errors++;
	goto cleanup;
}

