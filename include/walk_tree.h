#ifndef __WALK_TREE_H
#define __WALK_TREE_H

#define WALK_TREE_RECURSIVE	0x1
#define WALK_TREE_PHYSICAL	0x2
#define WALK_TREE_LOGICAL	0x4
#define WALK_TREE_DEREFERENCE	0x8

#define WALK_TREE_SYMLINK	0x10
#define WALK_TREE_FAILED	0x20

struct stat;

extern int walk_tree(const char *path, int walk_flags,
		     int (*func)(const char *, const struct stat *, int, void *),
		     void *arg);

#endif
