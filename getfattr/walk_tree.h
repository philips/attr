/*
 * Copyright (C) 2001 by Andreas Gruenbacher <a.gruenbacher@computer.org>
 *
 * TODO: should this be replaced by using nftw(3)?
 */
#include <sys/stat.h>

extern const char *progname;

#define WALK_FULL_LOGICAL	1	/* follow all symlinks */
#define WALK_HALF_LOGICAL	2	/* follow symlink arguments */
#define WALK_PHYSICAL		3	/* don't follow symlinks */

extern int walk_recurse;	/* recurse into sudirectories */
extern int walk_postorder;	/* walk tree in postorder */
extern int walk_symlinks;	/* follow symbolic links */

int walk_tree(const char *, int (*call)(const char *, struct stat *, void *), void *);
