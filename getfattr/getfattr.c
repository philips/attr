/*
 * Original getfattr.c:
 *	Copyright (C) 2001 by Andreas Gruenbacher <a.gruenbacher@computer.org>
 * Changes to use revised EA syscall interface:
 *	Copyright (C) 2001 by SGI XFS development <linux-xfs@oss.sgi.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <regex.h>
#include <xattr.h>
#include "walk_tree.h"

#include <locale.h>
#include <libintl.h>
#define _(String) gettext (String)

#define CMD_LINE_OPTIONS "ade:hln:r:svR5LPV"
#define CMD_LINE_SPEC "[-n name|-d] [-ahsvR5LPV] [-e en] [-r regex] path..."

int opt_dump;
int opt_symlink;
char *opt_encoding;
char opt_value_only;
int opt_strip_leading_slash = 1;

const char *progname;
int absolute_warning;
int header_printed;
int had_errors;
regex_t re;

int do_get_all(const char *, struct stat *, void *);
int do_get_one(const char *, struct stat *, void *);
int get_one(const char *, const char *);
const char *encode(const char *value, size_t *size);


void help(void)
{
	printf(_("%s %s -- get extended attributes\n"),
	       progname, VERSION);
	printf(_("Usage: %s %s\n"),
	         progname, CMD_LINE_SPEC);
	printf(_("\t-n name\tdump value of extended attribute `name'\n"
		"\t-a\tabsolute pathnames - leading '/' not stripped\n"
		"\t-d\tdump all extended attribute values\n"
		"\t-e en\tencode values (en = text|hex|base64)\n"
		"\t-l\tdump extended attribute values of a symlink\n"
		"\t-s\tdump extended system and user attribute values\n"
		"\t-v\tprint the attribute value(s) only\n"
		"\t-R\trecurse into subdirectories (-5 = post-order)\n"
	        "\t-L\tlogical walk, follow symbolic links\n"
		"\t-P\tphysical walk, do not follow symbolic links\n"
		"\t-V\tprint version and exit\n"
		"\t-h\tthis help text\n"));
}

int main(int argc, char *argv[])
{
	char	sys_pattern[] = "^system\\.|^user\\.";
	char	usr_pattern[] = "^user\\.";
	char	*pattern = usr_pattern;
	char	*name = NULL;

	progname = basename(argv[0]);
	setlocale(LC_ALL, "");

	while ((optopt = getopt(argc, argv, CMD_LINE_OPTIONS)) != -1) {
		switch(optopt) {
			case 'a': /* absolute names */
				opt_strip_leading_slash = 0;
				break;

			case 'd': /* dump attribute values */
				opt_dump = 1;
				break;

			case 'e':  /* encoding */
				if (strcmp(optarg, "text") != 0 &&
				    strcmp(optarg, "hex") != 0 &&
				    strcmp(optarg, "base64") != 0)
					goto synopsis;
				opt_encoding = optarg;
				break;

			case 'h':
				help();
				return 0;

			case 'l': /* dump attribute(s) of symlink itself */
				opt_symlink = 1;
				break;

			case 'n':  /* get named attribute */
				opt_dump = 1;
				name = optarg;
				break;

			case 'r':
				pattern = optarg;
				break;

			case 's':
				pattern = sys_pattern;
				break;

			case 'v':  /* get attribute values only */
				opt_value_only = 1;
				break;

			case 'H':
				walk_symlinks = WALK_HALF_LOGICAL;
				break;

			case 'L':
				walk_symlinks = WALK_FULL_LOGICAL;
				break;

			case 'P':
				walk_symlinks = WALK_PHYSICAL;
				break;

			case 'R':
				walk_recurse = 1;
				walk_postorder = 0;
				break;

			case '5':
				walk_recurse = 1;
				walk_postorder = 1;
				break;

			case 'V':
				printf("%s " VERSION "\n", progname);
				return 0;

			default:
				goto synopsis;
		}
	}
	if (optind >= argc)
		goto synopsis;

	if (regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB) != 0) {
		fprintf(stderr, _("%s: invalid regular expression \"%s\"\n"),
			progname, pattern);
		return 1;
	}

	while (optind < argc) {
		if (name)
			had_errors += walk_tree(argv[optind], do_get_one, name);
		else
			had_errors += walk_tree(argv[optind], do_get_all, NULL);
		optind++;
	}

	return (had_errors ? 1 : 0);

synopsis:
	fprintf(stderr, _("Usage: %s %s\n"
	                  "Try `%s -h' for more information.\n"),
		progname, CMD_LINE_SPEC, progname);
	return 2;
}

int do_getxattr(const char *path, const char *name, void *value, size_t size)
{
	if (opt_symlink)
		return lgetxattr(path, name, value, size);
	return getxattr(path, name, value, size);
}

int do_listxattr(const char *path, char *list, size_t size)
{
	if (opt_symlink)
		return llistxattr(path, list, size);
	return listxattr(path, list, size);
}

int high_water_alloc(void **buf, int *bufsize, int newsize)
{
	/*
	 * Goal here is to avoid unnecessary memory allocations by
	 * using static buffers which only grow when necessary.
	 * Size is increased in fixed size chunks (CHUNK_SIZE).
	 */
#define CHUNK_SIZE	256
	if (bufsize < newsize) {
		newsize = (newsize + CHUNK_SIZE-1) & ~(CHUNK_SIZE-1);
		*buf = realloc(newsize);
		if (!*buf) {
			perror(progname);
			had_errors++;
			*bufsize = 0;
			return 1;
		}
		*bufsize = newsize;
	}
	return 0;
}

int pstrcmp(const char **a, const char **b)
{
	return strcmp(*a, *b);
}

int do_get_all(const char *path, struct stat *stat, void *dummy)
{
	static char *list;
	static ssize_t bufsize;
	static char **names;
	static int ncount;

	char *v;
	ssize_t listsize;
	int n, count = 0;

	listsize = do_listxattr(path, NULL, 0);
	if (listsize < 0) {
		if (errno != ENOATTR && errno != ENOTSUP) {
			perror(path);
			had_errors++;
			return 1;
		}
	} else if (listsize > 0) {
		if (high_water_alloc(&list, &bufsize, listsize))
			return 1;

		listsize = do_listxattr(path, list, bufsize);
		if (listsize < 0) {
			perror(path);
			had_errors++;
			return 1;
		}

		for (v = list; v - list <= listsize; v += strlen(v)+1)
			if (regexec(&re, v, (size_t) 0, NULL, 0) == 0)
				count++;
		if (count) {
			n = count * sizeof(char *);
			if (high_water_alloc(&names, &ncount, n))
				return 1;
			n = 0;
			for (v = list; v - list <= listsize; v += strlen(v)+1)
				if (regexec(&re, v, (size_t) 0, NULL, 0) == 0)
					names[n++] = v;
			qsort(names, count, sizeof(char *),
			      (int (*)(const void *,const void *))pstrcmp);
		}
	}
	if (names) {
		header_printed = 0;
		for (n = 0; n < count; n++)
			get_one(path, names[n]);
		if (header_printed)
			puts("");
	}
	return 0;
}

int do_get_one(const char *path, struct stat *stat, void *name)
{
	int error;

	header_printed = 0;
	error = get_one(path, (const char *)name);
	if (header_printed)
		puts("");
	return error;
}

int get_one(const char *path, const char *name)
{
	static char *value;
	static size_t vsize;

	int error, lsize = 0;

	if (opt_dump || opt_value_only) {
		error = do_getxattr(path, name, NULL, 0);
		if (error < 0) {
			const char *strerr;
syscall_failed:
			if (!strncmp("system.", name, 7) && errno == ENOATTR)
				return 0;	/* expected */
			else if (errno == ENOATTR)
				strerr = _("No such attribute");
			else
				strerr = strerror(errno);
			fprintf(stderr, "%s: %s: %s\n", path, name, strerr);
			return 1;
		}
		if (high_water_alloc(&value, &vsize, error + 1))
			return 1;
		error = do_getxattr(path, name, value, vsize);
		if (error < 0)
			goto syscall_failed;
		lsize = error;
		value[lsize] = '\0';
	}

	if (opt_strip_leading_slash) {
		if (*path == '/') {
			if (!absolute_warning) {
				fprintf(stderr, _("%s: Removing leading '/' "
					"from absolute path names\n"),
					progname);
				absolute_warning = 1;
			}
			while (*path == '/')
				path++;
		} else if (*path == '.' && *(path+1) == '/')
			while (*++path == '/')
				/* nothing */ ;
		if (*path == '\0')
			path = ".";
	}

	if (!header_printed && !opt_value_only) {
		printf("# file: %s\n", path);
		header_printed = 1;
	}
	if (opt_value_only)
		puts(value);
	else if (lsize) {
		const char *e = encode(value, &lsize);

		if (e)
			printf("%s=%s\n", name, e);
	} else
		puts(name);

	return 0;
}

const char *encode(const char *value, size_t *size)
{
	static char *encoded = NULL, *e;
	
	if (encoded)
		free(encoded);
	if (opt_encoding == NULL || strcmp(opt_encoding, "text") == 0) {
		int n, extra = 0;

		for (e=(char *)value; e < value + *size; e++) {
			if (*e < 32 || *e >= 127)
				extra += 4;
			else if (*e == '\\' || *e == '"')
				extra++;
		}
		encoded = (char *)malloc(*size + extra + 3);
		if (!encoded) {
			perror(progname);
			had_errors++;
			return NULL;
		}
		e = encoded;
		*e++='"';
		for (n = 0; n < *size; n++, value++) {
			if (*value < 32 || *value >= 127) {
				*e++ = '\\';
				*e++ = '0' + ((unsigned char)*value >> 6);
				*e++ = '0' + (((unsigned char)*value & 070) >> 3);
				*e++ = '0' + ((unsigned char)*value & 07);
			} else if (*value == '\\' || *value == '"') {
				*e++ = '\\';
				*e++ = *value;
			} else {
				*e++ = *value;
			}
		}
		*e++ = '"';
		*e = '\0';
		*size = (e - encoded);
	} else if (strcmp(opt_encoding, "hex") == 0) {
		static const char *digits = "0123456789abcdef";
		int n;

		encoded = (char *)malloc(*size * 2 + 4);
		if (!encoded) {
			perror(progname);
			had_errors++;
			return NULL;
		}
		e = encoded;
		*e++='0'; *e++ = 'x';
		for (n = 0; n < *size; n++, value++) {
			*e++ = digits[((unsigned char)*value >> 4)];
			*e++ = digits[((unsigned char)*value & 0x0F)];
		}
		*e = '\0';
		*size = (e - encoded);
	} else if (strcmp(opt_encoding, "base64") == 0) {
		static const char *digits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
					    "ghijklmnopqrstuvwxyz0123456789+/";
		int n;

		encoded = (char *)malloc((*size + 2) / 3 * 4 + 1);
		if (!encoded) {
			perror(progname);
			had_errors++;
			return NULL;
		}
		e = encoded;
		*e++='0'; *e++ = 's';
		for (n=0; n + 2 < *size; n += 3) {
			*e++ = digits[(unsigned char)value[0] >> 2];
			*e++ = digits[(((unsigned char)value[0] & 0x03) << 4) |
			              (((unsigned char)value[1] & 0xF0) >> 4)];
			*e++ = digits[(((unsigned char)value[1] & 0x0F) << 2) |
			              ((unsigned char)value[2] >> 6)];
			*e++ = digits[(unsigned char)value[2] & 0x3F];
			value += 3;
		}
		if (*size - n == 2) {
			*e++ = digits[(unsigned char)value[0] >> 2];
			*e++ = digits[(((unsigned char)value[0] & 0x03) << 4) |
			              (((unsigned char)value[1] & 0xF0) >> 4)];
			*e++ = digits[((unsigned char)value[1] & 0x0F) << 2];
			*e++ = '=';
		} else if (*size - n == 1) {
			*e++ = digits[(unsigned char)value[0] >> 2];
			*e++ = digits[((unsigned char)value[0] & 0x03) << 4];
			*e++ = '=';
			*e++ = '=';
		}
		*e = '\0';
		*size = (e - encoded);
	}
	return encoded;
}
