/*
 * Setting.cpp
 *
 *  Created on: 2013年11月29日
 *      Author: zhangbo
 */

#include "Setting.h"
#include "windows/message.h"
#include "storage.h"

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


Setting::Setting() {
	// TODO Auto-generated constructor stub

}

Setting::~Setting() {
	// TODO Auto-generated destructor stub
}

void Setting::LoadConfigsFile(const char* file) {
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

void Setting::gppi(Properties *handle, const char *name, int def, int *i) {
	*i = handle->GetInteger(name, def);
}

void Setting::gpps(Properties *handle, const char *name, const char *def,
		char *val, int len) {
	strcpy(val, (handle->GetString(name, def)).c_str());
}

void Setting::gppmap(Properties *handle, const char *name, const char *def, char *val,
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

int Setting::key2val(const struct keyval *mapping, int nmaps, char *key) {
	int i;
	for (i = 0; i < nmaps; i++)
		if (!strcmp(mapping[i].s, key))
			return mapping[i].v;
	return -1;
}

void Setting::gppfile(void *handle, const char *name, Filename *result) {
	if (!read_setting_filename(handle, name, result))
		*result = platform_default_filename(name);
}

void Setting::gprefs(Properties *sesskey, const char *name, const char *def,
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

void Setting::gppfont(void *handle, const char *name, FontSpec *result) {
	if (!read_setting_fontspec(handle, name, result))
		*result = platform_default_fontspec(name);
}

void Setting::LoadProperties(Properties *sesskey, Config *cfg) {

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

void Setting::LoadPropertiesFile(const char* section, Config* cfg) {
	Properties prop;
	prop.SafeLoad(section);
	LoadProperties(&prop, cfg);
}
