/*
 * Original setfattr.c:
 *	Copyright (C) 2001 by Andreas Gruenbacher <a.gruenbacher@computer.org>
 * Changes to use revised EA syscall interface:
 *	Copyright (C) 2001 by SGI XFS development <linux-xfs@oss.sgi.com>
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>

#include <xattr.h>

#include <locale.h>
#include <libintl.h>
#define _(String) gettext (String)

#define CMD_LINE_OPTIONS "n:lv:x:B:Vh"
#define CMD_LINE_SPEC1 "{-n name|-x name} [-v value] [-lVh] file..."
#define CMD_LINE_SPEC2 "{-B filename} [-lVh] file..."

char *opt_name;
char *opt_remove;
char *opt_value;
int opt_restore;
int opt_symlink;

int had_errors;
const char *progname;

int do_set(const char *name, const char *value, const char *path);
const char *decode(const char *value, size_t *size);
int restore(FILE *file, const char *filename);
char *next_line(FILE *file);
int hex_digit(char c);
int base64_digit(char c);

int restore(FILE *file, const char *filename)
{
	char *path_p = NULL, *l;
	int line = 0, backup_line, status = 0;
	
	for(;;) {
		backup_line = line;
		while ((l = next_line(file)) != NULL && *l == '\0')
			line++;
		if (l == NULL)
			break;
		line++;
		if (strncmp(l, "# file: ", 8) != 0) {
			if (filename) {
				fprintf(stderr, _("%s: %s: No filename found "
				                  "in line %d, aborting\n"),
					progname, filename, backup_line);
			} else {
				fprintf(stderr, _("%s: No filename found in"
			                          "line %d of standard input, "
						  "aborting\n"),
					  progname, backup_line);
			}
			status = 1;
			goto cleanup;
		} else
			l += 8;
		if (path_p)
			free(path_p);
		path_p = (char *)malloc(strlen(l) + 1);
		if (!path_p) {
			status = 1;
			goto cleanup;
		}
		strcpy(path_p, l);

		while ((l = next_line(file)) != NULL && *l != '\0') {
			char *name = l, *value = strchr(l, '=');
			line++;
			if (value)
				*value++ = '\0';
			status = do_set(name, value, path_p);
		}
		if (l != NULL)
			line++;
	}

cleanup:
	if (path_p)
		free(path_p);
	return status;
}

void help(void)
{
	printf(_("%s %s -- set extended attributes\n"), progname, VERSION);
	printf(_("Usage: %s %s\n"), progname, CMD_LINE_SPEC1);
	printf(_("       %s %s\n"), progname, CMD_LINE_SPEC2);
	printf(_("Options:\n"));
	printf(_("\t-n name\tset value of extended attribute `name'\n"
		"\t-l\tset extended attribute values of a symlink\n"
		"\t-x name\tremove extended attribute `name'\n"
		"\t-v value\n\t\tvalue for extended attribute `name'\n"
		"\t-B filename\n\t\trestore extended attributes (inverse of `getfattr -sdlR')\n"
		"\t-V\tprint version and exit\n"
		"\t-h\tthis help text\n"));
}

char *next_line(FILE *file)
{
	static char line[_POSIX_PATH_MAX+32], *c;
	if (!fgets(line, sizeof(line), file))
		return NULL;

	c = strrchr(line, '\0');
	while (c > line && (*(c-1) == '\n' ||
			   *(c-1) == '\r')) {
		c--;
		*c = '\0';
	}
	return line;
}

int main(int argc, char *argv[])
{
	FILE *file;
	int status;

	progname = basename(argv[0]);

	while ((optopt = getopt(argc, argv, CMD_LINE_OPTIONS)) != -1) {
		switch(optopt) {
			case 'n':  /* attribute name */
				if (opt_remove)
					goto synopsis;
				opt_name = optarg;
				break;

			case 'l':  /* set attribute on symlink itself */
				opt_symlink = 1;
				break;

			case 'v':  /* attribute value */
				if (opt_value || opt_remove)
					goto synopsis;
				opt_value = optarg;
				break;

			case 'x':  /* remove attribute */
				if (opt_name)
					goto synopsis;
				opt_remove = optarg;
				break;

			case 'B':  /* restore */
				opt_restore = 1;
				if (strcmp(optarg, "-") == 0)
					file = stdin;
				else {
					file = fopen(optarg, "r");
					if (file == NULL) {
						fprintf(stderr, "%s: %s: %s\n",
						        progname, optarg,
						        strerror(errno));
						return 1;
					}
				}
				status = restore(file,
				               (file == stdin) ? NULL : optarg);
				if (file != stdin)
					fclose(file);
				if (status != 0)
					return 1;
				break;

			case 'V':
				printf("%s " VERSION "\n", progname);
				return 0;

			case 'h':
				help();
				return 0;

			default:
				goto synopsis;
		}
	}
	if (((opt_name && opt_remove) || (!opt_name && !opt_remove) ||
	    optind >= argc) && !opt_restore)
		goto synopsis;
	if (!opt_name) {
		opt_name = opt_remove;
		opt_value = NULL;
	}
	while (optind < argc) {
		do_set(opt_name, opt_value, argv[optind]);
		optind++;
	}

	return (had_errors ? 1 : 0);

synopsis:
	fprintf(stderr, _("Usage: %s %s\n"
			  "       %s %s\n"
	                  "Try `%s -h' for more information.\n"),
		progname, CMD_LINE_SPEC1, progname, CMD_LINE_SPEC2, progname);
	return 2;
}

int do_setxattr(const char *path, const char *name, void *value, size_t size)
{
	if (opt_symlink)	
		return lsetxattr(path, name, value, size, 0);
	return setxattr(path, name, value, size, 0);
}

int do_removexattr(const char *path, const char *name)
{
	if (opt_symlink)	
		return lremovexattr(path, name);
	return removexattr(path, name);
}

int do_set(const char *name, const char *value, const char *path)
{
	size_t size = 0;
	int error;

	if (value) {
		size = strlen(value);
		value = decode(value, &size);
		if (!value)
			return 1;
	}
	error = opt_remove? do_removexattr(path, name):
		do_setxattr(path, name, (void *)value, size);
	if (error < 0) {
		perror(path);
		had_errors++;
		return 1;
	}
	return 0;
}

const char *decode(const char *value, size_t *size)
{
	static char *decoded = NULL;

	if (decoded != NULL) {
		free(decoded);
		decoded = NULL;
	}
	if (value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
		const char *v = value+2, *end = value + *size;
		char *d;

		decoded = d = (char *)malloc(*size / 2);
		if (!decoded) {
			perror("");
			had_errors++;
			return NULL;
		}
		while (v < end) {
			int d1, d0;

			while (v < end && isspace(*v))
				v++;
			if (v == end)
				break;
			d1 = hex_digit(*v++);
			while (v < end && isspace(*v))
				v++;
			if (v == end) {
		bad_hex_encoding:
				fprintf(stderr, "bad input encoding\n");
				had_errors++;
				return NULL;
			}
			d0 = hex_digit(*v++);
			if (d1 < 0 || d0 < 0)
				goto bad_hex_encoding;
			*d++ = ((d1 << 4) | d0);
		}
		*size = d - decoded;
	} else if (value[0] == '0' && (value[1] == 's' || value[1] == 'S')) {
		const char *v = value+2, *end = value + *size;
		int d0, d1, d2, d3;
		char *d;

		decoded = d = (char *)malloc(*size / 4 * 3);
		if (!decoded) {
			perror("");
			had_errors++;
			return NULL;
		}
		for(;;) {
			while (v < end && isspace(*v))
				v++;
			if (v == end) {
				d0 = d1 = d2 = d3 = -2;
				break;
			}
			if (v + 4 > end) {
		bad_base64_encoding:
				fprintf(stderr, "bad input encoding\n");
				had_errors++;
				return NULL;
			}
			d0 = base64_digit(*v++);
			d1 = base64_digit(*v++);
			d2 = base64_digit(*v++);
			d3 = base64_digit(*v++);
			if (d0 < 0 || d1 < 0 || d2 < 0 || d3 < 0)
				break;

			*d++ = (char)((d0 << 2) | (d1 >> 4));
			*d++ = (char)((d1 << 4) | (d2 >> 2));
			*d++ = (char)((d2 << 6) | d3);
		}
		if (d0 == -2) {
			if (d1 != -2 || d2 != -2 || d3 != -2)
				goto bad_base64_encoding;
			goto base64_end;
		}
		if (d0 == -1 || d1 < 0 || d2 == -1 || d3 == -1)
			goto bad_base64_encoding;
		*d++ = (char)((d0 << 2) | (d1 >> 4));
		if (d2 != -2)
			*d++ = (char)((d1 << 4) | (d2 >> 2));
		else {
			if (d1 & 0x0F || d3 != -2)
				goto bad_base64_encoding;
			goto base64_end;
		}
		if (d3 != -2)
			*d++ = (char)((d2 << 6) | d3);
		else if (d2 & 0x03)
			goto bad_base64_encoding;
	base64_end:
		while (v < end && isspace(*v))
			v++;
		if (v + 4 <= end && *v == '=') {
			if (*++v != '=' || *++v != '=' || *++v != '=')
				goto bad_base64_encoding;
			v++;
		}
		while (v < end && isspace(*v))
			v++;
		if (v < end)
			goto bad_base64_encoding;
		*size = d - decoded;
	} else {
		const char *v = value, *end = value + *size;
		char *d;

		if (end > v+1 && *v == '"' && *(end-1) == '"') {
			v++;
			end--;
		}

		decoded = d = (char *)malloc(*size);
		if (!decoded) {
			perror("");
			had_errors++;
			return NULL;
		}

		while (v < end) {
			if (v[0] == '\\') {
				if (v[1] == '\\' || v[1] == '"') {
					*d++ = *++v; v++;
				} else if (v[1] >= '0' && v[1] <= '7') {
					int c = 0;
					v++;
					c = (*v++ - '0');
					if (*v >= '0' && *v <= '7')
						c = (c << 3) + (*v++ - '0');
					if (*v >= '0' && *v <= '7')
						c = (c << 3) + (*v++ - '0');
					*d++ = c;
				} else
					*d++ = *v++;
			} else
				*d++ = *v++;
		}
		*size = d - decoded;
	}
	return decoded;
}

int hex_digit(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else
		return -1;
}

int base64_digit(char c)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A';
	else if (c >= 'a' && c <= 'z')
		return 26 + c - 'a';
	else if (c >= '0' && c <= '9')
		return 52 + c - '0';
	else if (c == '+')
		return 62;
	else if (c == '/')
		return 63;
	else if (c == '=')
		return -2;
	else
		return -1;
}

