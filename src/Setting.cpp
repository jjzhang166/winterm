/*
 * Setting.cpp
 *
 *  Created on: 2013-3-14
 *      Author: xia
 */

#include "Setting.h"
#include <string.h>
#include "cpputils/Util.h"
#include "windows/message.h"
#include "storage.h"
using namespace std;

char filepath[MAX_PATH];
namespace terminal {

Setting::Setting() {
	// TODO Auto-generated constructor stub
}

Setting::~Setting() {
	// TODO Auto-generated destructor stub
}

void Setting::gppi(Properties *handle, char *name, int def, int *i) {
	*i = handle->GetInteger(name, def);
}

void Setting::gpps(Properties *handle, const char *name, const char *def,
		char *val, int len) {
	strcpy(val, (handle->GetString(name, def)).c_str());
}

void Setting::gppmap(Properties *handle, char *name, char *def, char *val,
		int len) {
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

static void gppfile(void *handle, const char *name, Filename *result) {
	if (!read_setting_filename(handle, name, result))
		*result = platform_default_filename(name);
}

void Setting::load_settings(char *section, Config * cfg) {
	sprintf(filepath, "%s\\%s.txt", currentpath, section);
	Properties prop;
	prop.SafeLoad(filepath);
	load_open_settings(&prop, cfg);
}

void Setting::save_settings(char *section, Config * cfg) {
	sprintf(filepath, "%s\\%s.txt", currentpath, section);
	Properties prop;
	prop.SafeLoad(filepath);
	save_open_settings(&prop, cfg);
	prop.SafeSave(filepath);
}

void Setting::write_setting_s(Properties *handle, const char *key,
		const char *value) {
	handle->PutString(key, value);
}

void Setting::write_setting_i(Properties *handle, const char *key, int value) {
	handle->PutInteger(key, value);
}

void Setting::write_setting_fontspec(Properties *handle, const char *name,
		FontSpec font) {
	char *settingname;

	write_setting_s(handle, name, font.name);
	settingname = dupcat(name, "IsBold", NULL);
	write_setting_i(handle, settingname, font.isbold);
	sfree(settingname);
	settingname = dupcat(name, "CharSet", NULL);
	write_setting_i(handle, settingname, font.charset);
	sfree(settingname);
	settingname = dupcat(name, "Height", NULL);
	write_setting_i(handle, settingname, font.height);
	sfree(settingname);
}

void Setting::wmap(Properties *handle, char const *key, char const *value,
		int len) {
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

static const char *val2key(const struct keyval *mapping, int nmaps, int val) {
	int i;
	for (i = 0; i < nmaps; i++)
		if (mapping[i].v == val)
			return mapping[i].s;
	return NULL;
}

void Setting::wprefs(Properties *sesskey, char *name,
		const struct keyval *mapping, int nvals, int *array) {
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

static int key2val(const struct keyval *mapping, int nmaps, char *key) {
	int i;
	for (i = 0; i < nmaps; i++)
		if (!strcmp(mapping[i].s, key))
			return mapping[i].v;
	return -1;
}

void Setting::gprefs(Properties *sesskey, char *name, char *def,
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

void Setting::save_reg_config() {
//	Properties prop;
//	prop.SafeLoad(configpath);
////	prop.Put("RegistKey", cfg.registkey);
//	prop.SafeSave(configpath);
}

void Setting::get_config() {
	Properties prop;
	prop.SafeLoad(configpath);
	gpps(&prop, "host", "192.168.1.118", cfg.updateip, NAME_LEN);
	gpps(&prop, "SysUser", "", cfg.sysuser, NAME_LEN);
	gpps(&prop, "SysPass", "", cfg.syspass, NAME_LEN);
	gpps(&prop, "Linkpath", "", cfg.linkpath, MAX_PATH);
	cfg.allowrepeat = prop.GetInteger("AllowRepeat", 0);
	cfg.printmode = prop.GetInteger("PrintMode", 3);
	cfg.updateport = prop.GetInteger("port", 8888);
	cfg.currentuser = prop.GetInteger("CurrentUser", 1);
	gpps(&prop, "FilePrintName", "LPT1", cfg.fprintname, 128);
	gpps(&prop, "SharePrinterName", "share", cfg.fpshare, 128);
}

void Setting::save_config() {
	Properties prop;
	prop.SafeLoad(configpath);
	prop.PutInteger("PrintMode", cfg.printmode);
	prop.PutString("FilePrintName", cfg.fprintname);
	prop.PutString("host", cfg.updateip);
	prop.PutInteger("port", cfg.updateport);
	prop.PutString("SysUser", cfg.sysuser);
	prop.PutString("SysPass", cfg.syspass);
	prop.PutInteger("CurrentUser", cfg.currentuser);
	prop.PutInteger("AllowRepeat", cfg.allowrepeat);
	prop.PutString("Linkpath", cfg.linkpath);
	prop.PutString("SharePrinterName", cfg.fpshare);
	prop.SafeSave(configpath);
	get_config();
}

void Setting::write_setting_filename(Properties *handle, const char *name,
		Filename result) {
	write_setting_s(handle, name, result.path);
}

void Setting::save_open_settings(Properties *sesskey, Config *cfg) {
	int i;
	char *p;
	write_setting_i(sesskey, "UseSelfSetY", cfg->useselfsety);
	write_setting_i(sesskey, "UseSelfSetZ", cfg->useselfsetz);
	write_setting_i(sesskey, "UseSelfSetX", cfg->useselfsetx);
	write_setting_i(sesskey, "UseSelfSetU", cfg->useselfsetu);
	write_setting_i(sesskey, "UseSelfSetW", cfg->useselfsetw);
	write_setting_i(sesskey, "UseSelfSetV", cfg->useselfsetv);
	write_setting_i(sesskey, "BandRateY", cfg->bandy);
	write_setting_i(sesskey, "BandRateZ", cfg->bandz);
	write_setting_i(sesskey, "BandRateX", cfg->bandx);
	write_setting_i(sesskey, "BandRateU", cfg->bandu);
	write_setting_i(sesskey, "BandRateW", cfg->bandw);
	write_setting_i(sesskey, "BandRateV", cfg->bandv);

	write_setting_filename(sesskey, "BellWaveFile", cfg->bell_wavefile);
	write_setting_fontspec(sesskey, "Font", cfg->font);
	write_setting_fontspec(sesskey, "BoldFont", cfg->boldfont);
	write_setting_fontspec(sesskey, "WideFont", cfg->widefont);
	write_setting_fontspec(sesskey, "WideBoldFont", cfg->wideboldfont);

	write_setting_i(sesskey, "Debug", cfg->debug);

	write_setting_s(sesskey, "ScreenNum", cfg->screennum);
	write_setting_i(sesskey, "AutoLogin", cfg->autologin);
	char loginptemp[NAME_LEN];
	char passptempp[NAME_LEN];
	memset(loginptemp, 0x00, NAME_LEN);
	memset(passptempp, 0x00, NAME_LEN);
	sprintf(loginptemp, "\"%s\"", cfg->loginprompt);
	sprintf(passptempp, "\"%s\"", cfg->passwordprompt);
	write_setting_s(sesskey, "LoginPrompt", loginptemp);
	write_setting_s(sesskey, "PasswordPrompt", passptempp);
	write_setting_s(sesskey, "AutoUserName", cfg->autousername);
	write_setting_s(sesskey, "AutoPassword", cfg->autopassword);
	write_setting_i(sesskey, "DeleteIsReturn", cfg->deleteisreturn);
	write_setting_i(sesskey, "AutoCopy", cfg->autocopy);
	write_setting_i(sesskey, "Com01y", cfg->com01y);
	write_setting_i(sesskey, "Com02z", cfg->com02z);
	write_setting_i(sesskey, "Com03x", cfg->com03x);
	write_setting_i(sesskey, "Com04u", cfg->com04u);
	write_setting_i(sesskey, "Com05w", cfg->com05w);
	write_setting_i(sesskey, "Com06v", cfg->com06v);
	write_setting_s(sesskey, "ComName01y", cfg->comname01y);
	write_setting_s(sesskey, "ComName02z", cfg->comname02z);
	write_setting_s(sesskey, "ComName03x", cfg->comname03x);
	write_setting_s(sesskey, "ComName04u", cfg->comname04u);
	write_setting_s(sesskey, "ComName05w", cfg->comname05w);
	write_setting_s(sesskey, "ComName06v", cfg->comname06v);

	write_setting_s(sesskey, "KeyCodeF01", cfg->keycodef[1]);
	write_setting_s(sesskey, "KeyCodeF02", cfg->keycodef[2]);
	write_setting_s(sesskey, "KeyCodeF03", cfg->keycodef[3]);
	write_setting_s(sesskey, "KeyCodeF04", cfg->keycodef[4]);
	write_setting_s(sesskey, "KeyCodeF05", cfg->keycodef[5]);
	write_setting_s(sesskey, "KeyCodeF06", cfg->keycodef[6]);
	write_setting_s(sesskey, "KeyCodeF07", cfg->keycodef[7]);
	write_setting_s(sesskey, "KeyCodeF08", cfg->keycodef[8]);
	write_setting_s(sesskey, "KeyCodeF09", cfg->keycodef[9]);
	write_setting_s(sesskey, "KeyCodeF10", cfg->keycodef[10]);
	write_setting_s(sesskey, "KeyCodeF11", cfg->keycodef[11]);
	write_setting_s(sesskey, "KeyCodeF12", cfg->keycodef[12]);
	write_setting_s(sesskey, "KeyCodeF01", cfg->keycodef[1]);

	write_setting_i(sesskey, "AllowShortCuts", cfg->allowshortcuts);
	write_setting_i(sesskey, "AllowFunction", cfg->allowfunction);
	write_setting_s(sesskey, "KeyCodeEsc", cfg->keycodeesc);
	write_setting_s(sesskey, "KeyCodeInsert", cfg->keycodeinsert);
	write_setting_s(sesskey, "KeyCodeDelete", cfg->keycodedelete);
	write_setting_s(sesskey, "KeyCodeHome", cfg->keycodehome);
	write_setting_s(sesskey, "KeyCodeSpace", cfg->keycodespace);
	write_setting_s(sesskey, "KeyCodeNumLock", cfg->keycodenumlock);
	write_setting_s(sesskey, "KeyCodePrintScreen", cfg->keycodeprintscreen);
	write_setting_s(sesskey, "KeyCodeScroll", cfg->keycodescroll);
	write_setting_s(sesskey, "KeyCodeEnd", cfg->keycodeend);
	write_setting_s(sesskey, "KeyCodePageUp", cfg->keycodepageup);
	write_setting_s(sesskey, "KeyCodePageDown", cfg->keycodepagedown);
	write_setting_s(sesskey, "KeyCodeAdd", cfg->keycodeadd);
	write_setting_s(sesskey, "KeyCodeSub", cfg->keycodesub);
	write_setting_s(sesskey, "KeyCodeDiv", cfg->keycodediv);
	write_setting_s(sesskey, "KeyCodeMult", cfg->keycodemult);
	write_setting_s(sesskey, "KeyCodePause", cfg->keycodepause);
	write_setting_s(sesskey, "KeyCodeUp", cfg->keycodeup);
	write_setting_s(sesskey, "KeyCodeDown", cfg->keycodedown);
	write_setting_s(sesskey, "KeyCodeLeft", cfg->keycodeleft);
	write_setting_s(sesskey, "KeyCodeRight", cfg->keycoderight);

	write_setting_i(sesskey, "Present", 1);
	write_setting_s(sesskey, "HostName", cfg->host);
	write_setting_filename(sesskey, "LogFileName", cfg->logfilename);
	write_setting_i(sesskey, "LogType", cfg->logtype);
	write_setting_i(sesskey, "LogFileClash", cfg->logxfovr);
	write_setting_i(sesskey, "LogFlush", cfg->logflush);
	write_setting_i(sesskey, "SSHLogOmitPasswords", cfg->logomitpass);
	write_setting_i(sesskey, "SSHLogOmitData", cfg->logomitdata);
	p = "raw";
	for (i = 0; backends[i].name != NULL; i++)
		if (backends[i].protocol == cfg->protocol) {
			p = backends[i].name;
			break;
		}
	write_setting_s(sesskey, "Protocol", p);
	write_setting_i(sesskey, "PortNumber", cfg->port);
	/* The CloseOnExit numbers are arranged in a different order from
	 * the standard FORCE_ON / FORCE_OFF / AUTO. */
	write_setting_i(sesskey, "CloseOnExit", (cfg->close_on_exit + 2) % 3);
	write_setting_i(sesskey, "WarnOnClose", !!cfg->warn_on_close);
	write_setting_i(sesskey, "PingInterval", cfg->ping_interval / 60); /* minutes */
	write_setting_i(sesskey, "PingIntervalSecs", cfg->ping_interval % 60); /* seconds */
	write_setting_i(sesskey, "TCPNoDelay", cfg->tcp_nodelay);
	write_setting_i(sesskey, "TCPKeepalives", cfg->tcp_keepalives);
	write_setting_s(sesskey, "TerminalType", cfg->termtype);
	write_setting_s(sesskey, "TerminalSpeed", cfg->termspeed);

	/* Address family selection */
	write_setting_i(sesskey, "AddressFamily", cfg->addressfamily);

	/* proxy settings */
	write_setting_s(sesskey, "ProxyExcludeList", cfg->proxy_exclude_list);
	write_setting_i(sesskey, "ProxyDNS", (cfg->proxy_dns + 2) % 3);
	write_setting_i(sesskey, "ProxyLocalhost", cfg->even_proxy_localhost);
	write_setting_i(sesskey, "ProxyMethod", cfg->proxy_type);
	write_setting_s(sesskey, "ProxyHost", cfg->proxy_host);
	write_setting_i(sesskey, "ProxyPort", cfg->proxy_port);
	write_setting_s(sesskey, "ProxyUsername", cfg->proxy_username);
	write_setting_s(sesskey, "ProxyPassword", cfg->proxy_password);
	write_setting_s(sesskey, "ProxyTelnetCommand", cfg->proxy_telnet_command);

	write_setting_s(sesskey, "UserName", cfg->username);
	write_setting_s(sesskey, "LocalUserName", cfg->localusername);
	write_setting_i(sesskey, "NoPTY", cfg->nopty);
	write_setting_i(sesskey, "Compression", cfg->compression);
	write_setting_i(sesskey, "TryAgent", cfg->tryagent);
	write_setting_i(sesskey, "AgentFwd", cfg->agentfwd);
	write_setting_i(sesskey, "ChangeUsername", cfg->change_username);

	write_setting_i(sesskey, "RekeyTime", cfg->ssh_rekey_time);
	write_setting_s(sesskey, "RekeyBytes", cfg->ssh_rekey_data);
	write_setting_i(sesskey, "SshNoAuth", cfg->ssh_no_userauth);
	write_setting_i(sesskey, "AuthTIS", cfg->try_tis_auth);
	write_setting_i(sesskey, "AuthKI", cfg->try_ki_auth);
	write_setting_i(sesskey, "SshNoShell", cfg->ssh_no_shell);
	write_setting_i(sesskey, "SshProt", cfg->sshprot);
	write_setting_i(sesskey, "SSH2DES", cfg->ssh2_des_cbc);
	write_setting_filename(sesskey, "PublicKeyFile", cfg->keyfile);
	write_setting_s(sesskey, "RemoteCommand", cfg->remote_cmd);
	write_setting_i(sesskey, "RFCEnviron", cfg->rfc_environ);
	write_setting_i(sesskey, "PassiveTelnet", cfg->passive_telnet);
	write_setting_i(sesskey, "BackspaceIsDelete", cfg->bksp_is_delete);
	write_setting_i(sesskey, "RXVTHomeEnd", cfg->rxvt_homeend);
	write_setting_i(sesskey, "LinuxFunctionKeys", cfg->funky_type);
	write_setting_i(sesskey, "NoApplicationKeys", cfg->no_applic_k);
	write_setting_i(sesskey, "NoApplicationCursors", cfg->no_applic_c);
	write_setting_i(sesskey, "NoMouseReporting", cfg->no_mouse_rep);
	write_setting_i(sesskey, "NoRemoteResize", cfg->no_remote_resize);
	write_setting_i(sesskey, "NoAltScreen", cfg->no_alt_screen);
	write_setting_i(sesskey, "NoRemoteWinTitle", cfg->no_remote_wintitle);
	write_setting_i(sesskey, "RemoteQTitleAction", cfg->remote_qtitle_action);
	write_setting_i(sesskey, "NoDBackspace", cfg->no_dbackspace);
	write_setting_i(sesskey, "NoRemoteCharset", cfg->no_remote_charset);
	write_setting_i(sesskey, "ApplicationCursorKeys", cfg->app_cursor);
	write_setting_i(sesskey, "ApplicationKeypad", cfg->app_keypad);
	write_setting_i(sesskey, "NetHackKeypad", cfg->nethack_keypad);
	write_setting_i(sesskey, "AltF4", cfg->alt_f4);
	write_setting_i(sesskey, "AltSpace", cfg->alt_space);
	write_setting_i(sesskey, "AltOnly", cfg->alt_only);
	write_setting_i(sesskey, "ComposeKey", cfg->compose_key);
	write_setting_i(sesskey, "CtrlAltKeys", cfg->ctrlaltkeys);
	write_setting_i(sesskey, "TelnetKey", cfg->telnet_keyboard);
	write_setting_i(sesskey, "TelnetRet", cfg->telnet_newline);
	write_setting_i(sesskey, "LocalEcho", cfg->localecho);
	write_setting_i(sesskey, "LocalEdit", cfg->localedit);
	write_setting_s(sesskey, "Answerback", cfg->answerback);
	write_setting_i(sesskey, "AlwaysOnTop", cfg->alwaysontop);
	write_setting_i(sesskey, "FullScreenOnAltEnter", cfg->fullscreenonaltenter);
	write_setting_i(sesskey, "HideMousePtr", cfg->hide_mouseptr);
	write_setting_i(sesskey, "SunkenEdge", cfg->sunken_edge);
	write_setting_i(sesskey, "WindowBorder", cfg->window_border);
	write_setting_i(sesskey, "CurType", cfg->cursor_type);
	write_setting_i(sesskey, "CurOn", cfg->cursor_on);
	write_setting_i(sesskey, "BlinkCur", cfg->blink_cur);

	write_setting_i(sesskey, "ScrollbackLines", cfg->savelines);
	write_setting_i(sesskey, "DECOriginMode", cfg->dec_om);
	write_setting_i(sesskey, "AutoWrapMode", cfg->wrap_mode);
	write_setting_i(sesskey, "LFImpliesCR", cfg->lfhascr);
	write_setting_i(sesskey, "DisableArabicShaping", cfg->arabicshaping);
	write_setting_i(sesskey, "DisableBidi", cfg->bidi);
	write_setting_i(sesskey, "WinNameAlways", cfg->win_name_always);
	write_setting_s(sesskey, "WinTitle", cfg->wintitle);
	write_setting_i(sesskey, "TermWidth", cfg->width);
	write_setting_i(sesskey, "TermHeight", cfg->height);

	write_setting_i(sesskey, "FontHeight", cfg->font.height);
	write_setting_i(sesskey, "FontWidth", cfg->fontwidth);
	write_setting_i(sesskey, "FontQuality", cfg->font_quality);
	write_setting_i(sesskey, "FontVTMode", cfg->vtmode);

	write_setting_i(sesskey, "UseSystemColours", cfg->system_colour);
	write_setting_i(sesskey, "TryPalette", cfg->try_palette);
	write_setting_i(sesskey, "ANSIColour", cfg->ansi_colour);
	write_setting_i(sesskey, "Xterm256Colour", cfg->xterm_256_colour);
	write_setting_i(sesskey, "BoldAsColour", cfg->bold_colour);

	for (i = 0; i < 22; i++) {
		char buf[20], buf2[30];
		sprintf(buf, "Colour%d", i);
		sprintf(buf2, "%d,%d,%d", cfg->colours[i][0], cfg->colours[i][1],
				cfg->colours[i][2]);
		write_setting_s(sesskey, buf, buf2);
	}
	write_setting_i(sesskey, "RawCNP", cfg->rawcnp);
	write_setting_i(sesskey, "PasteRTF", cfg->rtf_paste);
	write_setting_i(sesskey, "MouseIsXterm", cfg->mouse_is_xterm);
	write_setting_i(sesskey, "RectSelect", cfg->rect_select);
	write_setting_i(sesskey, "MouseOverride", cfg->mouse_override);
	for (i = 0; i < 256; i += 32) {
		char buf[20], buf2[256];
		int j;
		sprintf(buf, "Wordness%d", i);
		*buf2 = '\0';
		for (j = i; j < i + 32; j++) {
			sprintf(buf2 + strlen(buf2), "%s%d", (*buf2 ? "," : ""),
					cfg->wordness[j]);
		}
		write_setting_s(sesskey, buf, buf2);
	}
	write_setting_s(sesskey, "LineCodePage", cfg->line_codepage);
	write_setting_i(sesskey, "CJKAmbigWide", cfg->cjk_ambig_wide);
	write_setting_i(sesskey, "UTF8Override", cfg->utf8_override);
	write_setting_s(sesskey, "Printer", cfg->printer);
	write_setting_i(sesskey, "CapsLockCyr", cfg->xlat_capslockcyr);
	write_setting_i(sesskey, "ScrollBar", cfg->scrollbar);
	write_setting_i(sesskey, "ScrollBarFullScreen",
			cfg->scrollbar_in_fullscreen);
	write_setting_i(sesskey, "ScrollOnKey", cfg->scroll_on_key);
	write_setting_i(sesskey, "ScrollOnDisp", cfg->scroll_on_disp);
	write_setting_i(sesskey, "EraseToScrollback", cfg->erase_to_scrollback);
	write_setting_i(sesskey, "LockSize", cfg->resize_action);
	write_setting_i(sesskey, "BCE", cfg->bce);
	write_setting_i(sesskey, "BlinkText", cfg->blinktext);
	write_setting_i(sesskey, "X11Forward", cfg->x11_forward);
	write_setting_s(sesskey, "X11Display", cfg->x11_display);
	write_setting_i(sesskey, "X11AuthType", cfg->x11_auth);
	write_setting_i(sesskey, "LocalPortAcceptAll", cfg->lport_acceptall);
	write_setting_i(sesskey, "RemotePortAcceptAll", cfg->rport_acceptall);

	write_setting_i(sesskey, "BugIgnore1", 2 - cfg->sshbug_ignore1);
	write_setting_i(sesskey, "BugPlainPW1", 2 - cfg->sshbug_plainpw1);
	write_setting_i(sesskey, "BugRSA1", 2 - cfg->sshbug_rsa1);
	write_setting_i(sesskey, "BugHMAC2", 2 - cfg->sshbug_hmac2);
	write_setting_i(sesskey, "BugDeriveKey2", 2 - cfg->sshbug_derivekey2);
	write_setting_i(sesskey, "BugRSAPad2", 2 - cfg->sshbug_rsapad2);
	write_setting_i(sesskey, "BugPKSessID2", 2 - cfg->sshbug_pksessid2);
	write_setting_i(sesskey, "BugRekey2", 2 - cfg->sshbug_rekey2);
	write_setting_i(sesskey, "StampUtmp", cfg->stamp_utmp);
	write_setting_i(sesskey, "LoginShell", cfg->login_shell);
	write_setting_i(sesskey, "ScrollbarOnLeft", cfg->scrollbar_on_left);

	write_setting_i(sesskey, "ShadowBold", cfg->shadowbold);
	write_setting_i(sesskey, "ShadowBoldOffset", cfg->shadowboldoffset);
	write_setting_s(sesskey, "SerialLine", cfg->serline);
	write_setting_i(sesskey, "SerialSpeed", cfg->serspeed);
	write_setting_i(sesskey, "SerialDataBits", cfg->serdatabits);
	write_setting_i(sesskey, "SerialStopHalfbits", cfg->serstopbits);
	write_setting_i(sesskey, "SerialParity", cfg->serparity);
	write_setting_i(sesskey, "SerialFlowControl", cfg->serflow);
	write_setting_s(sesskey, "AdminPassword", cfg->adminpassword);
//	write_setting_i(sesskey, "ReturnPaper", cfg->returnpaper);
	wmap(sesskey, "TerminalModes", cfg->ttymodes, lenof(cfg->ttymodes));
	wmap(sesskey, "Environment", cfg->environmt, lenof(cfg->environmt));
	wmap(sesskey, "PortForwardings", cfg->portfwd, lenof(cfg->portfwd));
	wprefs(sesskey, "Cipher", ciphernames, CIPHER_MAX, cfg->ssh_cipherlist);
	wprefs(sesskey, "KEX", kexnames, KEX_MAX, cfg->ssh_kexlist);

	write_setting_i(sesskey, "Beep", cfg->beep);
	write_setting_i(sesskey, "BeepInd", cfg->beep_ind);
	write_setting_i(sesskey, "BellOverload", cfg->bellovl);
	write_setting_i(sesskey, "BellOverloadN", cfg->bellovl_n);
	write_setting_i(sesskey, "BellOverloadT", cfg->bellovl_t
#ifdef PUTTY_UNIX_H
			* 1000
#endif
			);
	write_setting_i(sesskey, "BellOverloadS", cfg->bellovl_s
#ifdef PUTTY_UNIX_H
			* 1000
#endif
			);

}

static void gppfont(void *handle, const char *name, FontSpec *result) {
	if (!read_setting_fontspec(handle, name, result))
		*result = platform_default_fontspec(name);
}
void Setting::load_open_settings(Properties *sesskey, Config *cfg) {

	int i;
	char prot[10];

	cfg->ssh_subsys = 0; /* FIXME: load this properly */
	cfg->remote_cmd_ptr = NULL;
	cfg->remote_cmd_ptr2 = NULL;
	cfg->ssh_nc_host[0] = '\0';

	gppfont(sesskey, "Font", &cfg->font);
	strcpy(cfg->font.name, FONT_NAME);
	cfg->font.charset = 134; //设置字库类型采用简体中文,西方字库为0

	gpps(sesskey, "BellWaveFile", "C:\\WINDOWS\\Media\\ding.wav",
			cfg->bell_wavefile.path, FILENAME_MAX);
	gpps(sesskey, "LogFileName", "putty.log", cfg->logfilename.path,
			FILENAME_MAX);

	gpps(sesskey, "PublicKeyFile", "", cfg->keyfile.path, FILENAME_MAX);

	gppfont(sesskey, "BoldFont", &cfg->boldfont);
	gppfont(sesskey, "WideFont", &cfg->widefont);
	gppfont(sesskey, "WideBoldFont", &cfg->wideboldfont);

	gppi(sesskey, "UseSelfSetY", 0, &cfg->useselfsety);
	gppi(sesskey, "UseSelfSetZ", 0, &cfg->useselfsetz);
	gppi(sesskey, "UseSelfSetX", 0, &cfg->useselfsetx);
	gppi(sesskey, "UseSelfSetU", 0, &cfg->useselfsetu);
	gppi(sesskey, "UseSelfSetW", 0, &cfg->useselfsetw);
	gppi(sesskey, "UseSelfSetV", 0, &cfg->useselfsetv);
	gppi(sesskey, "BandRateY", 9600, &cfg->bandy);
	gppi(sesskey, "BandRateZ", 9600, &cfg->bandz);
	gppi(sesskey, "BandRateX", 9600, &cfg->bandx);
	gppi(sesskey, "BandRateU", 9600, &cfg->bandu);
	gppi(sesskey, "BandRateW", 9600, &cfg->bandw);
	gppi(sesskey, "BandRateV", 9600, &cfg->bandv);

	gppi(sesskey, "Debug", 0, &cfg->debug);
	gppi(sesskey, "Beep", 1, &cfg->beep);
	gppi(sesskey, "BeepInd", 0, &cfg->beep_ind);

	/* pedantic compiler tells me I can't use &cfg->beep as an int * :-) */

	gppi(sesskey, "BellOverload", 1, &cfg->bellovl);
	gppi(sesskey, "BellOverloadN", 5, &cfg->bellovl_n);
	gppi(sesskey, "BellOverloadT", 2 * TICKSPERSEC, &i);
	cfg->bellovl_t = i
#ifdef PUTTY_UNIX_H
			/ 1000
#endif
			;
	gppi(sesskey, "BellOverloadS", 5 * TICKSPERSEC, &i);
	cfg->bellovl_s = i
#ifdef PUTTY_UNIX_H
			/ 1000
#endif
			;

//	gppi(sesskey, "ReturnPaper", 6, &cfg->returnpaper);
	gpps(sesskey, "AdminPassword", "", cfg->adminpassword, 128);
	gpps(sesskey, "ScreenNum", "", cfg->screennum, NAME_LEN);
	gppi(sesskey, "AutoLogin", 0, &cfg->autologin);
	char loginptemp[NAME_LEN];
	char passptemp[NAME_LEN];
	memset(loginptemp, 0x00, NAME_LEN);
	memset(passptemp, 0x00, NAME_LEN);
	gpps(sesskey, "LoginPrompt", "", loginptemp, NAME_LEN);
	gpps(sesskey, "PasswordPrompt", "", passptemp, NAME_LEN);
	if (strlen(loginptemp) >= 2) {
		memcpy(cfg->loginprompt, loginptemp + 1, strlen(loginptemp) - 2);
	}
	if (strlen(passptemp) >= 2) {
		memcpy(cfg->passwordprompt, passptemp + 1, strlen(passptemp) - 2);
	}
//	gpps(sesskey, "LoginPrompt", "", cfg->loginprompt, NAME_LEN);
//	gpps(sesskey, "PasswordPrompt", "", cfg->passwordprompt, NAME_LEN);
	gpps(sesskey, "AutoUserName", "", cfg->autousername, NAME_LEN);
	gpps(sesskey, "AutoPassword", "", cfg->autopassword, NAME_LEN);
	gppi(sesskey, "CurOn", 1, &cfg->cursor_on);
	gppi(sesskey, "DeleteIsReturn", 0, &cfg->deleteisreturn);
	gppi(sesskey, "AutoCopy", 0, &cfg->autocopy);
	gppi(sesskey, "Com01y", 4, &cfg->com01y);
	gppi(sesskey, "Com02z", 3, &cfg->com02z);
	gppi(sesskey, "Com03x", 5, &cfg->com03x);
	gppi(sesskey, "Com04u", 1, &cfg->com04u);
	gppi(sesskey, "Com05w", 6, &cfg->com05w);
	gppi(sesskey, "Com06v", 6, &cfg->com06v);
	gpps(sesskey, "ComName01y", COMNAME1, cfg->comname01y, NAME_LEN);
	gpps(sesskey, "ComName02z", COMNAME2, cfg->comname02z, NAME_LEN);
	gpps(sesskey, "ComName03x", COMNAME3, cfg->comname03x, NAME_LEN);
	gpps(sesskey, "ComName04u", COMNAME4, cfg->comname04u, NAME_LEN);
	gpps(sesskey, "ComName05w", COMNAME5, cfg->comname05w, NAME_LEN);
	gpps(sesskey, "ComName06v", COMNAME6, cfg->comname06v, NAME_LEN);

	gpps(sesskey, "KeyCodeF01", "^[OP", cfg->keycodef[1], KEYCODEFXX_MAX_SIZE);
	gpps(sesskey, "KeyCodeF02", "^[OQ", cfg->keycodef[2], KEYCODEFXX_MAX_SIZE);
	gpps(sesskey, "KeyCodeF03", "^[OR", cfg->keycodef[3], KEYCODEFXX_MAX_SIZE);
	gpps(sesskey, "KeyCodeF04", "^[OS", cfg->keycodef[4], KEYCODEFXX_MAX_SIZE);
	gpps(sesskey, "KeyCodeF05", "^[Ot", cfg->keycodef[5], KEYCODEFXX_MAX_SIZE);
	gpps(sesskey, "KeyCodeF06", "^[Ou", cfg->keycodef[6], KEYCODEFXX_MAX_SIZE);
	gpps(sesskey, "KeyCodeF07", "^[Ov", cfg->keycodef[7], KEYCODEFXX_MAX_SIZE);
	gpps(sesskey, "KeyCodeF08", "^[Ol", cfg->keycodef[8], KEYCODEFXX_MAX_SIZE);
	gpps(sesskey, "KeyCodeF09", "^[Ow", cfg->keycodef[9], KEYCODEFXX_MAX_SIZE);
	gpps(sesskey, "KeyCodeF10", "^[Ox", cfg->keycodef[10], KEYCODEFXX_MAX_SIZE);
	gpps(sesskey, "KeyCodeF11", "^[Oy", cfg->keycodef[11], KEYCODEFXX_MAX_SIZE);
	gpps(sesskey, "KeyCodeF12", "", cfg->keycodef[12], KEYCODEFXX_MAX_SIZE);

	gppi(sesskey, "AllowShortCuts", 1, &(cfg->allowshortcuts));
	gppi(sesskey, "AllowFunction", 0, &(cfg->allowfunction));
	gpps(sesskey, "KeyCodeEsc", "^[", cfg->keycodeesc, KEY_LEN);
	gpps(sesskey, "KeyCodeInsert", "^[[1~", cfg->keycodeinsert, KEY_LEN);
	gpps(sesskey, "KeyCodeDelete", "^8", cfg->keycodedelete, KEY_LEN);
	gpps(sesskey, "KeyCodeHome", "^[[2~", cfg->keycodehome, KEY_LEN);
	gpps(sesskey, "KeyCodeSpace", "^H", cfg->keycodespace, KEY_LEN);
	gpps(sesskey, "KeyCodeNumLock", "^[OP", cfg->keycodenumlock, KEY_LEN);
	gpps(sesskey, "KeyCodePrintScreen", "^[i", cfg->keycodeprintscreen,
			KEY_LEN);
	gpps(sesskey, "KeyCodeScroll", "", cfg->keycodescroll, KEY_LEN);
	gpps(sesskey, "KeyCodeEnd", "^[[5~", cfg->keycodeend, KEY_LEN);
	gpps(sesskey, "KeyCodePageUp", "^[[3~", cfg->keycodepageup, KEY_LEN);
	gpps(sesskey, "KeyCodePageDown", "^[[6~", cfg->keycodepagedown, KEY_LEN);
	gpps(sesskey, "KeyCodeAdd", "^[Om", cfg->keycodeadd, KEY_LEN);
	gpps(sesskey, "KeyCodeSub", "^[OS", cfg->keycodesub, KEY_LEN);
	gpps(sesskey, "KeyCodeDiv", "^[OQ", cfg->keycodediv, KEY_LEN);
	gpps(sesskey, "KeyCodeMult", "^[OR", cfg->keycodemult, KEY_LEN);
	gpps(sesskey, "KeyCodePause", "", cfg->keycodepause, KEY_LEN);
	gpps(sesskey, "KeyCodeUp", "", cfg->keycodeup, KEY_LEN);
	gpps(sesskey, "KeyCodeDown", "", cfg->keycodedown, KEY_LEN);
	gpps(sesskey, "KeyCodeLeft", "", cfg->keycodeleft, KEY_LEN);
	gpps(sesskey, "KeyCodeRight", "", cfg->keycoderight, KEY_LEN);

	gppi(sesskey, "FontHeight", 28, &cfg->font.height);
	gppi(sesskey, "FontWidth", 12, &cfg->fontwidth);
	gppi(sesskey, "FontQuality", FQ_DEFAULT, &cfg->font_quality);

	gpps(sesskey, "HostName", "127.0.0.1", cfg->host, sizeof(cfg->host));

	gppi(sesskey, "LogType", LGTYP_NONE, &cfg->logtype);
	gppi(sesskey, "LogFileClash", LGXF_ASK, &cfg->logxfovr);
	gppi(sesskey, "LogFlush", 1, &cfg->logflush);
	gppi(sesskey, "SSHLogOmitPasswords", 1, &cfg->logomitpass);
	gppi(sesskey, "SSHLogOmitData", 0, &cfg->logomitdata);

	gpps(sesskey, "Protocol", "default", prot, 10);
	cfg->protocol = default_protocol;
	cfg->port = default_port;
	for (i = 0; backends[i].name != NULL; i++)
		if (!strcmp(prot, backends[i].name)) {
			cfg->protocol = backends[i].protocol;
			gppi(sesskey, "PortNumber", default_port, &cfg->port);
			break;
		}

	/* Address family selection */
	gppi(sesskey, "AddressFamily", ADDRTYPE_UNSPEC, &cfg->addressfamily);

	/* The CloseOnExit numbers are arranged in a different order from
	 * the standard FORCE_ON / FORCE_OFF / AUTO. */
	gppi(sesskey, "CloseOnExit", 0, &i);
	cfg->close_on_exit = (i + 1) % 3;
	gppi(sesskey, "WarnOnClose", 1, &cfg->warn_on_close);
	{
		/* This is two values for backward compatibility with 0.50/0.51 */
		int pingmin, pingsec;
		gppi(sesskey, "PingInterval", 0, &pingmin);
		gppi(sesskey, "PingIntervalSecs", 30, &pingsec);
		cfg->ping_interval = pingmin * 60 + pingsec;
	}
	gppi(sesskey, "TCPNoDelay", 1, &cfg->tcp_nodelay);
	gppi(sesskey, "TCPKeepalives", 1, &cfg->tcp_keepalives);
	gpps(sesskey, "TerminalType", "vt100", cfg->termtype,
			sizeof(cfg->termtype));
	gpps(sesskey, "TerminalSpeed", "38400,38400", cfg->termspeed,
			sizeof(cfg->termspeed));

	/* proxy settings */
	gpps(sesskey, "ProxyExcludeList", "", cfg->proxy_exclude_list,
			sizeof(cfg->proxy_exclude_list));
	gppi(sesskey, "ProxyDNS", 1, &i);
	cfg->proxy_dns = (i + 1) % 3;
	gppi(sesskey, "ProxyLocalhost", 0, &cfg->even_proxy_localhost);
	gppi(sesskey, "ProxyMethod", -1, &cfg->proxy_type);
	if (cfg->proxy_type == -1) {
		int i;
		gppi(sesskey, "ProxyType", 0, &i);
		if (i == 0)
			cfg->proxy_type = PROXY_NONE;
		else if (i == 1)
			cfg->proxy_type = PROXY_HTTP;
		else if (i == 3)
			cfg->proxy_type = PROXY_TELNET;
		else if (i == 4)
			cfg->proxy_type = PROXY_CMD;
		else {
			gppi(sesskey, "ProxySOCKSVersion", 5, &i);
			if (i == 5)
				cfg->proxy_type = PROXY_SOCKS5;
			else
				cfg->proxy_type = PROXY_SOCKS4;
		}
	}
	gpps(sesskey, "ProxyHost", "proxy", cfg->proxy_host,
			sizeof(cfg->proxy_host));
	gppi(sesskey, "ProxyPort", 80, &cfg->proxy_port);
	gpps(sesskey, "ProxyUsername", "", cfg->proxy_username,
			sizeof(cfg->proxy_username));
	gpps(sesskey, "ProxyPassword", "", cfg->proxy_password,
			sizeof(cfg->proxy_password));
	gpps(sesskey, "ProxyTelnetCommand", "connect %host %port\\n",
			cfg->proxy_telnet_command, sizeof(cfg->proxy_telnet_command));

	gpps(sesskey, "UserName", "", cfg->username, sizeof(cfg->username));
	gpps(sesskey, "LocalUserName", "", cfg->localusername,
			sizeof(cfg->localusername));
	gppi(sesskey, "NoPTY", 0, &cfg->nopty);
	gppi(sesskey, "Compression", 0, &cfg->compression);
	gppi(sesskey, "TryAgent", 1, &cfg->tryagent);
	gppi(sesskey, "AgentFwd", 0, &cfg->agentfwd);
	gppi(sesskey, "ChangeUsername", 0, &cfg->change_username);

	gppi(sesskey, "RekeyTime", 60, &cfg->ssh_rekey_time);
	gpps(sesskey, "RekeyBytes", "1G", cfg->ssh_rekey_data,
			sizeof(cfg->ssh_rekey_data));
	gppi(sesskey, "SshProt", 2, &cfg->sshprot);
	gppi(sesskey, "SSH2DES", 0, &cfg->ssh2_des_cbc);
	gppi(sesskey, "SshNoAuth", 0, &cfg->ssh_no_userauth);
	gppi(sesskey, "AuthTIS", 0, &cfg->try_tis_auth);
	gppi(sesskey, "AuthKI", 1, &cfg->try_ki_auth);
	gppi(sesskey, "SshNoShell", 0, &cfg->ssh_no_shell);

	gpps(sesskey, "RemoteCommand", "", cfg->remote_cmd,
			sizeof(cfg->remote_cmd));
	gppi(sesskey, "RFCEnviron", 0, &cfg->rfc_environ);
	gppi(sesskey, "PassiveTelnet", 0, &cfg->passive_telnet);
	gppi(sesskey, "BackspaceIsDelete", 1, &cfg->bksp_is_delete);
	gppi(sesskey, "RXVTHomeEnd", 0, &cfg->rxvt_homeend);
	gppi(sesskey, "LinuxFunctionKeys", FUNKY_VT400, &cfg->funky_type);
	gppi(sesskey, "NoApplicationKeys", 1, &cfg->no_applic_k);
	gppi(sesskey, "NoApplicationCursors", 0, &cfg->no_applic_c);
	gppi(sesskey, "NoMouseReporting", 0, &cfg->no_mouse_rep);
	gppi(sesskey, "NoRemoteResize", 0, &cfg->no_remote_resize);
	gppi(sesskey, "NoAltScreen", 0, &cfg->no_alt_screen);
	gppi(sesskey, "NoRemoteWinTitle", 0, &cfg->no_remote_wintitle);
	{
		/* Backward compatibility */
		int no_remote_qtitle;
		gppi(sesskey, "NoRemoteQTitle", 1, &no_remote_qtitle);
		/* We deliberately interpret the old setting of "no response" as
		 * "empty string". This changes the behaviour, but hopefully for
		 * the better; the user can always recover the old behaviour. */
		gppi(sesskey, "RemoteQTitleAction",
				no_remote_qtitle ? TITLE_EMPTY : TITLE_REAL,
				&cfg->remote_qtitle_action);
	}
	gppi(sesskey, "NoDBackspace", 0, &cfg->no_dbackspace);
	gppi(sesskey, "NoRemoteCharset", 0, &cfg->no_remote_charset);
	gppi(sesskey, "ApplicationCursorKeys", 0, &cfg->app_cursor);
	gppi(sesskey, "ApplicationKeypad", 1, &cfg->app_keypad);
	gppi(sesskey, "NetHackKeypad", 0, &cfg->nethack_keypad);
	gppi(sesskey, "AltF4", 1, &cfg->alt_f4);
	gppi(sesskey, "AltSpace", 0, &cfg->alt_space);
	gppi(sesskey, "AltOnly", 0, &cfg->alt_only);
	gppi(sesskey, "ComposeKey", 0, &cfg->compose_key);
	gppi(sesskey, "CtrlAltKeys", 1, &cfg->ctrlaltkeys);
	gppi(sesskey, "TelnetKey", 0, &cfg->telnet_keyboard);
	gppi(sesskey, "TelnetRet", 1, &cfg->telnet_newline);
	gppi(sesskey, "LocalEcho", AUTO, &cfg->localecho);
	gppi(sesskey, "LocalEdit", AUTO, &cfg->localedit);
	gpps(sesskey, "Answerback", "PuTTY", cfg->answerback,
			sizeof(cfg->answerback));
	gppi(sesskey, "AlwaysOnTop", 0, &cfg->alwaysontop);
	gppi(sesskey, "FullScreenOnAltEnter", 0, &cfg->fullscreenonaltenter);
	gppi(sesskey, "HideMousePtr", 0, &cfg->hide_mouseptr);
	gppi(sesskey, "SunkenEdge", 0, &cfg->sunken_edge);
	gppi(sesskey, "WindowBorder", 1, &cfg->window_border);
	gppi(sesskey, "CurType", 0, &cfg->cursor_type);
	gppi(sesskey, "BlinkCur", 0, &cfg->blink_cur);

	gppi(sesskey, "ScrollbackLines", 200, &cfg->savelines);
	gppi(sesskey, "DECOriginMode", 0, &cfg->dec_om);
	gppi(sesskey, "AutoWrapMode", 1, &cfg->wrap_mode);
	gppi(sesskey, "LFImpliesCR", 0, &cfg->lfhascr);
	gppi(sesskey, "DisableArabicShaping", 0, &cfg->arabicshaping);
	gppi(sesskey, "DisableBidi", 0, &cfg->bidi);
	gppi(sesskey, "WinNameAlways", 1, &cfg->win_name_always);
	gpps(sesskey, "WinTitle", "", cfg->wintitle, sizeof(cfg->wintitle));
	gppi(sesskey, "TermWidth", 80, &cfg->width);
	gppi(sesskey, "TermHeight", 24, &cfg->height);

	gppi(sesskey, "FontHeight", 28, &cfg->font.height);
	gppi(sesskey, "FontWidth", 12, &cfg->fontwidth);
	gppi(sesskey, "FontQuality", FQ_DEFAULT, &cfg->font_quality);

	gppi(sesskey, "FontVTMode", VT_UNICODE, (int *) &cfg->vtmode);
	gppi(sesskey, "UseSystemColours", 0, &cfg->system_colour);
	gppi(sesskey, "TryPalette", 0, &cfg->try_palette);
	gppi(sesskey, "ANSIColour", 1, &cfg->ansi_colour);
	gppi(sesskey, "Xterm256Colour", 1, &cfg->xterm_256_colour);
	gppi(sesskey, "BoldAsColour", 1, &cfg->bold_colour);

	for (i = 0; i < 22; i++) {
		static const char * const defaults[] = { "255,255,0", "255,255,255",
				"0,128,64", "85,85,85", "0,0,0", "0,255,0", "0,0,0", "85,85,85",
				"187,0,0", "255,85,85", "0,187,0", "85,255,85", "187,187,0",
				"255,255,85", "0,0,187", "85,85,255", "187,0,187", "255,85,255",
				"0,187,187", "85,255,255", "187,187,187", "255,255,255" };
		char buf[20], buf2[30];
		int c0, c1, c2;
		sprintf(buf, "Colour%d", i);
		gpps(sesskey, buf, defaults[i], buf2, sizeof(buf2));
		if (sscanf(buf2, "%d,%d,%d", &c0, &c1, &c2) == 3) {
			cfg->colours[i][0] = c0;
			cfg->colours[i][1] = c1;
			cfg->colours[i][2] = c2;
		}
	}
	gppi(sesskey, "RawCNP", 0, &cfg->rawcnp);
	gppi(sesskey, "PasteRTF", 0, &cfg->rtf_paste);
	gppi(sesskey, "MouseIsXterm", 0, &cfg->mouse_is_xterm);
	gppi(sesskey, "RectSelect", 0, &cfg->rect_select);
	gppi(sesskey, "MouseOverride", 1, &cfg->mouse_override);
	for (i = 0; i < 256; i += 32) {
		static const char * const defaults[] =
				{
						"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
						"0,1,2,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1",
						"1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,2",
						"1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1",
						"1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1",
						"1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1",
						"2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2",
						"2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2" };
		char buf[20], buf2[256], *p;
		int j;
		sprintf(buf, "Wordness%d", i);
		gpps(sesskey, buf, defaults[i / 32], buf2, sizeof(buf2));
		p = buf2;
		for (j = i; j < i + 32; j++) {
			char *q = p;
			while (*p && *p != ',')
				p++;
			if (*p == ',')
				*p++ = '\0';
			cfg->wordness[j] = atoi(q);
		}
	}
	/*
	 * The empty default for LineCodePage will be converted later
	 * into a plausible default for the locale.
	 */
	gpps(sesskey, "LineCodePage", "", cfg->line_codepage,
			sizeof(cfg->line_codepage));
	gppi(sesskey, "CJKAmbigWide", 0, &cfg->cjk_ambig_wide);
	gppi(sesskey, "UTF8Override", 1, &cfg->utf8_override);
	gpps(sesskey, "Printer", "", cfg->printer, sizeof(cfg->printer));
	gppi(sesskey, "CapsLockCyr", 0, &cfg->xlat_capslockcyr);
	gppi(sesskey, "ScrollBar", 0, &cfg->scrollbar);
	gppi(sesskey, "ScrollBarFullScreen", 0, &cfg->scrollbar_in_fullscreen);
	gppi(sesskey, "ScrollOnKey", 0, &cfg->scroll_on_key);
	gppi(sesskey, "ScrollOnDisp", 0, &cfg->scroll_on_disp);
	gppi(sesskey, "EraseToScrollback", 0, &cfg->erase_to_scrollback);
	gppi(sesskey, "LockSize", 0, &cfg->resize_action);
	gppi(sesskey, "BCE", 1, &cfg->bce);
	gppi(sesskey, "BlinkText", 0, &cfg->blinktext);
	gppi(sesskey, "X11Forward", 0, &cfg->x11_forward);
	gpps(sesskey, "X11Display", "", cfg->x11_display, sizeof(cfg->x11_display));
	gppi(sesskey, "X11AuthType", X11_MIT, &cfg->x11_auth);

	gppi(sesskey, "LocalPortAcceptAll", 0, &cfg->lport_acceptall);
	gppi(sesskey, "RemotePortAcceptAll", 0, &cfg->rport_acceptall);

	gppi(sesskey, "BugIgnore1", 0, &i);
	cfg->sshbug_ignore1 = 2 - i;
	gppi(sesskey, "BugPlainPW1", 0, &i);
	cfg->sshbug_plainpw1 = 2 - i;
	gppi(sesskey, "BugRSA1", 0, &i);
	cfg->sshbug_rsa1 = 2 - i;
	{
		int i;
		gppi(sesskey, "BugHMAC2", 0, &i);
		cfg->sshbug_hmac2 = 2 - i;
		if (cfg->sshbug_hmac2 == AUTO) {
			gppi(sesskey, "BuggyMAC", 0, &i);
			if (i == 1)
				cfg->sshbug_hmac2 = FORCE_ON;
		}
	}
	gppi(sesskey, "BugDeriveKey2", 0, &i);
	cfg->sshbug_derivekey2 = 2 - i;
	gppi(sesskey, "BugRSAPad2", 0, &i);
	cfg->sshbug_rsapad2 = 2 - i;
	gppi(sesskey, "BugPKSessID2", 0, &i);
	cfg->sshbug_pksessid2 = 2 - i;
	gppi(sesskey, "BugRekey2", 0, &i);
	cfg->sshbug_rekey2 = 2 - i;
	gppi(sesskey, "StampUtmp", 1, &cfg->stamp_utmp);
	gppi(sesskey, "LoginShell", 1, &cfg->login_shell);
	gppi(sesskey, "ScrollbarOnLeft", 0, &cfg->scrollbar_on_left);
	gppi(sesskey, "ShadowBold", 0, &cfg->shadowbold);

	gppi(sesskey, "ShadowBoldOffset", 1, &cfg->shadowboldoffset);
	gpps(sesskey, "SerialLine", "", cfg->serline, sizeof(cfg->serline));
	gppi(sesskey, "SerialSpeed", 9600, &cfg->serspeed);
	gppi(sesskey, "SerialDataBits", 8, &cfg->serdatabits);
	gppi(sesskey, "SerialStopHalfbits", 2, &cfg->serstopbits);
	gppi(sesskey, "SerialParity", SER_PAR_NONE, &cfg->serparity);
	gppi(sesskey, "SerialFlowControl", SER_FLOW_XONXOFF, &cfg->serflow);

//	gpps(sesskey, "BellWaveFile", "",cfg->bell_wavefile.path,sizeof(cfg->bell_wavefile.path));

	gppmap(sesskey, "PortForwardings", "", cfg->portfwd, lenof(cfg->portfwd));
	gppmap(sesskey, "Environment", "", cfg->environmt, lenof(cfg->environmt));
	{
		/* This hardcodes a big set of defaults in any new saved
		 * sessions. Let's hope we don't change our mind. */
		int i;
		char *def = dupstr("");
		/* Default: all set to "auto" */
		for (i = 0; ttymodes[i]; i++) {
			char *def2 = dupprintf("%s%s=A,", def, ttymodes[i]);
			sfree(def);
			def = def2;
		}
		gppmap(sesskey, "TerminalModes", def, cfg->ttymodes,
				lenof(cfg->ttymodes));
		sfree(def);
	}

	gprefs(sesskey, "Cipher", "\0", ciphernames, CIPHER_MAX,
			cfg->ssh_cipherlist);

	{
		/* Backward-compatibility: we used to have an option to
		 * disable gex under the "bugs" panel after one report of
		 * a server which offered it then choked, but we never got
		 * a server version string or any other reports. */
		char *default_kexes;
		gppi(sesskey, "BugDHGEx2", 0, &i);
		i = 2 - i;
		if (i == FORCE_ON)
			default_kexes = "dh-group14-sha1,dh-group1-sha1,WARN,dh-gex-sha1";
		else
			default_kexes = "dh-gex-sha1,dh-group14-sha1,dh-group1-sha1,WARN";
		gprefs(sesskey, "KEX", default_kexes, kexnames, KEX_MAX,
				cfg->ssh_kexlist);
	}
}

} /* namespace terminal */
