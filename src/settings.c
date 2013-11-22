/*
 * settings.c: read and write saved sessions. (platform-independent)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "putty.h"
#include "storage.h"
#include "windows/message.h"
/*
 * Tables of string <-> enum value mappings
 */
struct keyval {
	char *s;
	int v;
};

/* The cipher order given here is the default order. */
static const struct keyval ciphernames[] = { { "aes", CIPHER_AES }, {
		"blowfish", CIPHER_BLOWFISH }, { "3des", CIPHER_3DES }, { "WARN",
		CIPHER_WARN }, { "arcfour", CIPHER_ARCFOUR }, { "des", CIPHER_DES } };

static const struct keyval kexnames[] = { { "dh-gex-sha1", KEX_DHGEX }, {
		"dh-group14-sha1", KEX_DHGROUP14 }, { "dh-group1-sha1", KEX_DHGROUP1 },
		{ "WARN", KEX_WARN } };

/*
 * All the terminal modes that we know about for the "TerminalModes"
 * setting. (Also used by config.c for the drop-down list.)
 * This is currently precisely the same as the set in ssh.c, but could
 * in principle differ if other backends started to support tty modes
 * (e.g., the pty backend).
 */
const char * const ttymodes[] = { "INTR", "QUIT", "ERASE", "KILL", "EOF", "EOL",
		"EOL2", "START", "STOP", "SUSP", "DSUSP", "REPRINT", "WERASE", "LNEXT",
		"FLUSH", "SWTCH", "STATUS", "DISCARD", "IGNPAR", "PARMRK", "INPCK",
		"ISTRIP", "INLCR", "IGNCR", "ICRNL", "IUCLC", "IXON", "IXANY", "IXOFF",
		"IMAXBEL", "ISIG", "ICANON", "XCASE", "ECHO", "ECHOE", "ECHOK",
		"ECHONL", "NOFLSH", "TOSTOP", "IEXTEN", "ECHOCTL", "ECHOKE", "PENDIN",
		"OPOST", "OLCUC", "ONLCR", "OCRNL", "ONOCR", "ONLRET", "CS7", "CS8",
		"PARENB", "PARODD", NULL };

static void gpps(void *handle, const char *name, const char *def, char *val,
		int len) {
	if (!read_setting_s(handle, name, val, len)) {
		char *pdef;

		pdef = platform_default_s(name);
		if (pdef) {
			strncpy(val, pdef, len);
			sfree(pdef);
		} else {
			strncpy(val, def, len);
		}

		val[len - 1] = '\0';
	}
}

/*
 * gppfont and gppfile cannot have local defaults, since the very
 * format of a Filename or Font is platform-dependent. So the
 * platform-dependent functions MUST return some sort of value.
 */
//static void gppfont(void *handle, const char *name, FontSpec *result) {
//	if (!read_setting_fontspec(handle, name, result))
//		*result = platform_default_fontspec(name);
//}
//static void gppfile(void *handle, const char *name, Filename *result) {
//	if (!read_setting_filename(handle, name, result))
//		*result = platform_default_filename(name);
//}
static void gppi(void *handle, char *name, int def, int *i) {
	def = platform_default_i(name, def);
	*i = read_setting_i(handle, name, def);
}

/*
 * Read a set of name-value pairs in the format we occasionally use:
 *   NAME\tVALUE\0NAME\tVALUE\0\0 in memory
 *   NAME=VALUE,NAME=VALUE, in storage
 * `def' is in the storage format.
 */
static void gppmap(void *handle, char *name, char *def, char *val, int len) {
	char *buf = snewn(2*len, char), *p, *q;
	gpps(handle, name, def, buf, 2 * len);
	p = buf;
	q = val;
	while (*p) {
		while (*p && *p != ',') {
			int c = *p++;
			if (c == '=')
				c = '\t';
			if (c == '\\')
				c = *p++;
			*q++ = c;
		}
		if (*p == ',')
			p++;
		*q++ = '\0';
	}
	*q = '\0';
	sfree(buf);
}

/*
 * Write a set of name/value pairs in the above format.
 */
static void wmap(void *handle, char const *key, char const *value, int len) {
	char *buf = snewn(2*len, char), *p;
	const char *q;
	p = buf;
	q = value;
	while (*q) {
		while (*q) {
			int c = *q++;
			if (c == '=' || c == ',' || c == '\\')
				*p++ = '\\';
			if (c == '\t')
				c = '=';
			*p++ = c;
		}
		*p++ = ',';
		q++;
	}
	*p = '\0';
	write_setting_s(handle, key, buf);
	sfree(buf);
}

static int key2val(const struct keyval *mapping, int nmaps, char *key) {
	int i;
	for (i = 0; i < nmaps; i++)
		if (!strcmp(mapping[i].s, key))
			return mapping[i].v;
	return -1;
}

static const char *val2key(const struct keyval *mapping, int nmaps, int val) {
	int i;
	for (i = 0; i < nmaps; i++)
		if (mapping[i].v == val)
			return mapping[i].s;
	return NULL;
}

/*
 * Helper function to parse a comma-separated list of strings into
 * a preference list array of values. Any missing values are added
 * to the end and duplicates are weeded.
 * XXX: assumes vals in 'mapping' are small +ve integers
 */
static void gprefs(void *sesskey, char *name, char *def,
		const struct keyval *mapping, int nvals, int *array) {
	char commalist[80];
	char *tokarg = commalist;
	int n;
	unsigned long seen = 0; /* bitmap for weeding dups etc */
	gpps(sesskey, name, def, commalist, sizeof(commalist));

	/* Grotty parsing of commalist. */
	n = 0;
	do {
		int v;
		char *key;
		key = strtok(tokarg, ","); /* sorry */
		tokarg = NULL;
		if (!key)
			break;
		if (((v = key2val(mapping, nvals, key)) != -1) && !(seen & 1 << v)) {
			array[n] = v;
			n++;
			seen |= 1 << v;
		}
	} while (n < nvals);
	/* Add any missing values (backward compatibility ect). */
	{
		int i;
		for (i = 0; i < nvals; i++) {
			if (mapping[i].v >= 32) {
				return;
			}
			// assert(mapping[i].v < 32);
			if (!(seen & 1 << mapping[i].v)) {
				array[n] = mapping[i].v;
				n++;
			}
		}
	}
}

/* 
 * Write out a preference list.
 */
static void wprefs(void *sesskey, char *name, const struct keyval *mapping,
		int nvals, int *array) {
	char buf[80] = ""; /* XXX assumed big enough */
	int l = sizeof(buf) - 1, i;
	buf[l] = '\0';
	for (i = 0; l > 0 && i < nvals; i++) {
		const char *s = val2key(mapping, nvals, array[i]);
		if (s) {
			int sl = strlen(s);
			if (i > 0) {
				strncat(buf, ",", l);
				l--;
			}
			strncat(buf, s, l);
			l -= sl;
		}
	}
	write_setting_s(sesskey, name, buf);
}

void save_open_settingsc(void *sesskey, Config *cfg) {

}
//
char *save_settings(char *section, Config * cfg) {
	void *sesskey;
	char *errmsg;
	sesskey = open_settings_w(section, &errmsg);
	if (!sesskey)
		return errmsg;
	save_open_settingsc(sesskey, cfg);
	close_settings_w(sesskey);
	return NULL;
}

void load_open_settingsc(void *sesskey, Config *cfg) {
	int i;
//	gppfile(sesskey, "LogFileName", &cfg->logfilename);
//
//	gppfile(sesskey, "PublicKeyFile", &cfg->keyfile);

}

void load_settings(char *section, Config * cfg) {
	void *sesskey;

	sesskey = open_settings_r(section);
	load_open_settingsc(sesskey, cfg);
	close_settings_r(sesskey);
}

void do_defaults(char *session, Config * cfg) {
	load_settings(session, cfg);

}

static int sessioncmp(const void *av, const void *bv) {
	const char *a = *(const char * const *) av;
	const char *b = *(const char * const *) bv;

	/*
	 * Alphabetical order, except that "Default Settings" is a
	 * special case and comes first.
	 */
	if (!strcmp(a, DEFAULT_SETTINGS))
		return -1; /* a comes first */
	if (!strcmp(b, DEFAULT_SETTINGS))
		return +1; /* b comes first */
	/*
	 * FIXME: perhaps we should ignore the first & in determining
	 * sort order.
	 */
	return strcmp(a, b); /* otherwise, compare normally */
}

void get_sesslist(struct sesslist *list, int allocate) {
	char otherbuf[2048];
	int buflen, bufsize, i;
	char *p, *ret;
	void *handle;

	if (allocate) {

		buflen = bufsize = 0;
		list->buffer = NULL;
		if ((handle = enum_settings_start()) != NULL) {
			do {
				ret = enum_settings_next(handle, otherbuf, sizeof(otherbuf));
				if (ret) {
					int len = strlen(otherbuf) + 1;
					if (bufsize < buflen + len) {
						bufsize = buflen + len + 2048;
						list->buffer = sresize(list->buffer, bufsize, char);
					}
					strcpy(list->buffer + buflen, otherbuf);
					buflen += strlen(list->buffer + buflen) + 1;
				}
			} while (ret);
			enum_settings_finish(handle);
		}
		list->buffer = sresize(list->buffer, buflen + 1, char);
		list->buffer[buflen] = '\0';

		/*
		 * Now set up the list of sessions. Note that "Default
		 * Settings" must always be claimed to exist, even if it
		 * doesn't really.
		 */

		p = list->buffer;
		list->nsessions = 1; /* "Default Settings" counts as one */
		while (*p) {
			if (strcmp(p, DEFAULT_SETTINGS))
				list->nsessions++;
			while (*p)
				p++;
			p++;
		}

		list->sessions = snewn(list->nsessions + 1, char *);
		list->sessions[0] = DEFAULT_SETTINGS;
		p = list->buffer;
		i = 1;
		while (*p) {
			if (strcmp(p, DEFAULT_SETTINGS))
				list->sessions[i++] = p;
			while (*p)
				p++;
			p++;
		}

		qsort(list->sessions, i, sizeof(char *), sessioncmp);
	} else {
		sfree(list->buffer);
		sfree(list->sessions);
		list->buffer = NULL;
		list->sessions = NULL;
	}
}
