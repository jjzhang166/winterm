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

	static void LoadConfigsFile(const char* file);

	static void LoadPropertiesFile(const char* section, Config* cfg);

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
};

#endif /* SETTING_H_ */
