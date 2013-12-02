/*
 * Setting.h
 *
 *  Created on: 2013年11月29日
 *      Author: zhangbo
 */

#ifndef SETTING_H_
#define SETTING_H_

#include "putty.h"

#include <cpputils/Properties.h>

struct keyval {
	char *s;
	int v;
};

class Setting {
public:
	Setting();
	virtual ~Setting();

	static void LoadConfigsFile(const char* file, Config* cfg);

	static void LoadPropertiesFile(const char* section, Config* cfg);

	static void SaveConfigsFile(const char* file, Config* cfg);

	static void SavePropertiesFile(const char* section, Config* cfg);

private:
	static int key2val(const struct keyval *mapping, int nmaps, char *key);
	static void gppi(Properties *handle, const char *name, int def, int *i);
	static void gppmap(Properties *handle, const char *name, const char *def, char *val,
			int len);
	static void gpps(Properties *handle, const char *name, const char *def,
			char *val, int len);
	static void gppfile(void *handle, const char *name, Filename *result);
	static void gppfont(void *handle, const char *name, FontSpec *result);
	static void gprefs(Properties *sesskey, const char *name, const char *def,
			const struct keyval *mapping, int nvals, int *array);
	static void LoadProperties(Properties *sesskey, Config *cfg);

	static void write_setting_s(Properties *handle, const char *key,
			const char *value);
	static void write_setting_i(Properties *handle, const char *key, int value);
	static void wmap(Properties *handle, char const *key, char const *value,
			int len);
	static void wprefs(Properties *sesskey, char *name,
			const struct keyval *mapping, int nvals, int *array);
	static void write_setting_fontspec(Properties *handle, const char *name,
			FontSpec font);
	static void write_setting_filename(Properties *handle, const char *name,
			Filename result);
	static void SaveProperties(Properties *sesskey, Config *cfg);
};

#endif /* SETTING_H_ */
