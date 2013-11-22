/*
 * Setting.h
 *
 *  Created on: 2013-3-14
 *      Author: xia
 */

#ifndef SETTING_H_
#define SETTING_H_
#include "cpputils/Properties.h"
#include "putty.h"
namespace terminal {
struct keyval {
	char *s;
	int v;
};
static const struct keyval ciphernames[] = { { "aes", CIPHER_AES }, {
		"blowfish", CIPHER_BLOWFISH }, { "3des", CIPHER_3DES }, { "WARN",
		CIPHER_WARN }, { "arcfour", CIPHER_ARCFOUR }, { "des", CIPHER_DES } };

static const struct keyval kexnames[] = { { "dh-gex-sha1", KEX_DHGEX }, {
		"dh-group14-sha1", KEX_DHGROUP14 }, { "dh-group1-sha1", KEX_DHGROUP1 },
		{ "WARN", KEX_WARN } };
class Setting {
private:

public:
	Setting();
	virtual ~Setting();
	static void gppi(Properties *handle, char *name, int def, int *i);
	static void gppmap(Properties *handle, char *name, char *def, char *val,
			int len);
	static void gpps(Properties *handle, const char *name, const char *def,
			char *val, int len);
	static void write_setting_s(Properties *handle, const char *key,
			const char *value);
	static void write_setting_i(Properties *handle, const char *key, int value);
	static void wmap(Properties *handle, char const *key, char const *value,
			int len);
	static void wprefs(Properties *sesskey, char *name,
			const struct keyval *mapping, int nvals, int *array);
	static void load_settings(char *section, Config * cfg);
	static void load_open_settings(Properties *sesskey, Config *cfg);
	static void save_settings(char *section, Config * cfg);
	static void save_open_settings(Properties *sesskey, Config *cfg);
	static void gprefs(Properties *sesskey, char *name, char *def,
			const struct keyval *mapping, int nvals, int *array);
	static void write_setting_fontspec(Properties *handle, const char *name,
			FontSpec font);
	static void write_setting_filename(Properties *handle, const char *name,
			Filename result);
	static void get_config();
	static void save_reg_config();
	static void get_reg_default(char *section);
	static void save_config();
};

} /* namespace terminal */
#endif /* SETTING_H_ */
