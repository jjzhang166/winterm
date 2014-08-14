#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "putty.h"
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include <io.h> 
#include <conio.h>
#include "util/log.h"
extern Backend *back;
extern void *backhandle;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define	IAC	255		       /* interpret as command: *///表示命令开始标记，即该字节后面的内容当作命令来解释#define	DONT	254		       /* you are not to use option *///表示的是协商选项，发送方想让对方禁止某个子选项#define	DO	253		       /* please, you use option *///表示的是协商选项，发送方想让对方激活某个子选项#define	WONT	252		       /* I won't use option *///表示的是协商选项，发送方想主动禁止某个子选项#define	WILL	251		       /* I will use option *///该命令表示的是协商选项,发送方想主动激活某个子选项#define	SB	250		       /* interpret as subnegotiation *///表示子选项开始#define	SE	240		       /* end sub negotiation *///子选项结束#define GA      249		       /* you may reverse the line *///表示继续进行#define EL      248		       /* erase the current line *///删除行#define EC      247		       /* erase the current character *///表示的是转义字符#define	AYT	246		       /* are you there *///表示对方是否还在运行#define	AO	245		       /* abort output--but let prog finish *///异常终止进程#define	IP	244		       /* interrupt process--permanently *///表示的是中断进程#define	BREAK	243		       /* break *///该命令表示的是中断#define DM      242		       /* data mark--for connect. cleaning *///该命令表示的是数据标记#define NOP     241		       /* nop *///表示空操作#define EOR     239		       /* end of record (transparent mode) *///表示的是记录标识符#define ABORT   238		       /* Abort process *///表示非正常终止当前进程#define SUSP    237		       /* Suspend process *///表示挂起当前进程#define xEOF    236		       /* End of file: EOF is already used... *///表示文件结束符//telnet 协议的子选项定义#define TELOPT_BINARY	0	       /* 8-bit data path */#define TELOPT_ECHO	1	       /* echo *///表示服务器是否回显#define	TELOPT_RCP	2	       /* prepare to reconnect */#define	TELOPT_SGA	3	       /* suppress go ahead *///抑制继续进行#define	TELOPT_NAMS	4	       /* approximate message size */#define	TELOPT_STATUS	5	       /* give status *///状态#define	TELOPT_TM	6	       /* timing mark *///定时标记#define	TELOPT_RCTE	7	       /* remote controlled transmission and echo */#define TELOPT_NAOL 	8	       /* negotiate about output line width */#define TELOPT_NAOP 	9	       /* negotiate about output page size */
#define TELOPT_NAOCRD	10	       /* negotiate about CR disposition */
#define TELOPT_NAOHTS	11	       /* negotiate about horizontal tabstops */
#define TELOPT_NAOHTD	12	       /* negotiate about horizontal tab disposition */
#define TELOPT_NAOFFD	13	       /* negotiate about formfeed disposition */
#define TELOPT_NAOVTS	14	       /* negotiate about vertical tab stops */
#define TELOPT_NAOVTD	15	       /* negotiate about vertical tab disposition */
#define TELOPT_NAOLFD	16	       /* negotiate about output LF disposition */
#define TELOPT_XASCII	17	       /* extended ascic character set */
#define	TELOPT_LOGOUT	18	       /* force logout */
#define	TELOPT_BM	19	       /* byte macro */
#define	TELOPT_DET	20	       /* data entry terminal */
#define	TELOPT_SUPDUP	21	       /* supdup protocol */
#define	TELOPT_SUPDUPOUTPUT 22	       /* supdup output */
#define	TELOPT_SNDLOC	23	       /* send location */
#define	TELOPT_TTYPE	24	       /* terminal type *///终端类型#define	TELOPT_EOR	25	       /* end or record */#define	TELOPT_TUID	26	       /* TACACS user identification */#define	TELOPT_OUTMRK	27	       /* output marking */
#define	TELOPT_TTYLOC	28	       /* terminal location number */
#define	TELOPT_3270REGIME 29	       /* 3270 regime */
#define	TELOPT_X3PAD	30	       /* X.3 PAD */
#define	TELOPT_NAWS	31	       /* window size *///窗口大小#define	TELOPT_TSPEED	32	       /* terminal speed *///终端速率#define	TELOPT_LFLOW	33	       /* remote flow control *///选程流量控制#define TELOPT_LINEMODE	34	       /* Linemode option *///执行方式#define TELOPT_XDISPLOC	35	       /* X Display Location */#define TELOPT_OLD_ENVIRON 36	       /* Old - Environment variables *///环境变量#define	TELOPT_AUTHENTICATION 37       /* Authenticate */#define	TELOPT_ENCRYPT	38	       /* Encryption option */#define TELOPT_NEW_ENVIRON 39	       /* New - Environment variables */
#define TELOPT_TN3270E	40	       /* TN3270 enhancements */
#define TELOPT_XAUTH	41
#define TELOPT_CHARSET	42	       /* Character set */
#define TELOPT_RSP	43	       /* Remote serial port */
#define TELOPT_COM_PORT_OPTION 44      /* Com port control */
#define TELOPT_SLE	45	       /* Suppress local echo */
#define TELOPT_STARTTLS	46	       /* Start TLS */
#define TELOPT_KERMIT	47	       /* Automatic Kermit file transfer */
#define TELOPT_SEND_URL	48
#define TELOPT_FORWARD_X 49
#define TELOPT_PRAGMA_LOGON	138
#define TELOPT_SSPI_LOGON	139
#define TELOPT_PRAGMA_HEARTBEAT	140
#define	TELOPT_EXOPL	255	       /* extended-options-list */

#define	TELQUAL_IS	0	       /* option is... */
#define	TELQUAL_SEND	1	       /* send option */
#define	TELQUAL_INFO	2	       /* ENVIRON: informational version of IS */
#define BSD_VAR 1
#define BSD_VALUE 0
#define RFC_VAR 0
#define RFC_VALUE 1

#define CR 13
#define LF 10
#define NUL 0

#define iswritable(x) \
	( (x) != IAC && \
	      (telnet->opt_states[o_we_bin.index] == ACTIVE || (x) != CR))

static int telnet_send(void *handle, char *buf, int len);

static char *telopt(int opt) {
#define i(x) if (opt == TELOPT_ ## x) return #x;
	i(BINARY);
	i(ECHO);
	i(RCP);
	i(SGA);
	i(NAMS);
	i(STATUS);
	i(TM);
	i(RCTE);
	i(NAOL);
	i(NAOP);
	i(NAOCRD);
	i(NAOHTS);
	i(NAOHTD);
	i(NAOFFD);
	i(NAOVTS);
	i(NAOVTD);
	i(NAOLFD);
	i(XASCII);
	i(LOGOUT);
	i(BM);
	i(DET);
	i(SUPDUP);
	i(SUPDUPOUTPUT);
	i(SNDLOC);
	i(TTYPE);
	i(EOR);
	i(TUID);
	i(OUTMRK);
	i(TTYLOC);
	i(X3PAD);
	i(NAWS);
	i(TSPEED);
	i(LFLOW);
	i(LINEMODE);
	i(XDISPLOC);
	i(OLD_ENVIRON);
	i(AUTHENTICATION);
	i(ENCRYPT);
	i(NEW_ENVIRON);
	i(TN3270E);
	i(XAUTH);
	i(CHARSET);
	i(RSP);
	i(COM_PORT_OPTION);
	i(SLE);
	i(STARTTLS);
	i(KERMIT);
	i(SEND_URL);
	i(FORWARD_X);
	i(PRAGMA_LOGON);
	i(SSPI_LOGON);
	i(PRAGMA_HEARTBEAT);
	i(EXOPL);
#undef i
	return "<unknown>";
}

static void telnet_size(void *handle, int width, int height);

struct Opt {
	int send; /* what we initially send */
	int nsend; /* -ve send if requested to stop it */
	int ack, nak; /* +ve and -ve acknowledgements */
	int option; /* the option code */
	int index; /* index into telnet->opt_states[] */
	enum {
		REQUESTED, ACTIVE, INACTIVE, REALLY_INACTIVE
	} initial_state;
};

enum {
	OPTINDEX_NAWS,
	OPTINDEX_TSPEED,
	OPTINDEX_TTYPE,
	OPTINDEX_OENV,
	OPTINDEX_NENV,
	OPTINDEX_ECHO,
	OPTINDEX_WE_SGA,
	OPTINDEX_THEY_SGA,
	OPTINDEX_WE_BIN,
	OPTINDEX_THEY_BIN,
	NUM_OPTS
};

static const struct Opt o_naws = { WILL, WONT, DO, DONT, TELOPT_NAWS,
		OPTINDEX_NAWS, REQUESTED };
static const struct Opt o_tspeed = { WILL, WONT, DO, DONT, TELOPT_TSPEED,
		OPTINDEX_TSPEED, REALLY_INACTIVE };
static const struct Opt o_ttype = { WILL, WONT, DO, DONT, TELOPT_TTYPE,
		OPTINDEX_TTYPE, REQUESTED };
static const struct Opt o_oenv = { WILL, WONT, DO, DONT, TELOPT_OLD_ENVIRON,
		OPTINDEX_OENV, INACTIVE };
static const struct Opt o_nenv = { WILL, WONT, DO, DONT, TELOPT_NEW_ENVIRON,
		OPTINDEX_NENV, REALLY_INACTIVE };
static const struct Opt o_echo = { DO, DONT, WILL, WONT, TELOPT_ECHO,
		OPTINDEX_ECHO, REQUESTED };
static const struct Opt o_we_sga = { WILL, WONT, DO, DONT, TELOPT_SGA,
		OPTINDEX_WE_SGA, REQUESTED };
static const struct Opt o_they_sga = { DO, DONT, WILL, WONT, TELOPT_SGA,
		OPTINDEX_THEY_SGA, REQUESTED };
static const struct Opt o_we_bin = { WILL, WONT, DO, DONT, TELOPT_BINARY,
		OPTINDEX_WE_BIN, INACTIVE };
static const struct Opt o_they_bin = { DO, DONT, WILL, WONT, TELOPT_BINARY,
		OPTINDEX_THEY_BIN, INACTIVE };

static const struct Opt * const opts[] = { &o_naws, &o_tspeed, &o_ttype,
		&o_oenv, &o_nenv, &o_echo, &o_we_sga, &o_they_sga, &o_we_bin,
		&o_they_bin, NULL };

typedef struct telnet_tag {
	const struct plug_function_table *fn;
	/* the above field _must_ be first in the structure */

	Socket s;

	void *frontend;
	void *ldisc;
	int term_width, term_height;

	int opt_states[NUM_OPTS];

	int echoing, editing;
	int activated;
	int bufsize;
	int in_synch;
	int sb_opt, sb_len;
	unsigned char *sb_buf;
	int sb_size;

	enum {
		TOP_LEVEL,
		SEENIAC,
		SEENWILL,
		SEENWONT,
		SEENDO,
		SEENDONT,
		SEENSB,
		SUBNEGOT,
		SUBNEG_IAC,
		SEENCR,
	} state;

	Config cfg;

	Pinger pinger;
}*Telnet;

#define TELNET_MAX_BACKLOG 4096

#define SB_DELTA 1024

static void c_write(Telnet telnet, char *buf, int len) {
	int backlog;
	backlog = from_backend(telnet->frontend, 0, buf, len);
	sk_set_frozen(telnet->s, backlog > TELNET_MAX_BACKLOG);
}

static void log_option(Telnet telnet, char *sender, int cmd, int option) {
	char *buf;
	/*
	 * The strange-looking "<?""?>" below is there to avoid a
	 * trigraph - a double question mark followed by > maps to a
	 * closing brace character!
	 */
	buf = dupprintf("%s:\t%s %s", sender,
			(cmd == WILL ? "WILL" : cmd == WONT ? "WONT" : cmd == DO ? "DO" :
				cmd == DONT ? "DONT" : "<?" "?>"), telopt(option));
	logevent(telnet->frontend, buf);
	sfree(buf);
}

static void send_opt(Telnet telnet, int cmd, int option) {
	unsigned char b[3];

	b[0] = IAC;
	b[1] = cmd;
	b[2] = option;
	telnet->bufsize = sk_write(telnet->s, (char * )b, 3);
	log_option(telnet, "client", cmd, option);
}

static void deactivate_option(Telnet telnet, const struct Opt *o) {
	if (telnet->opt_states[o->index] == REQUESTED
			|| telnet->opt_states[o->index] == ACTIVE)
		send_opt(telnet, o->nsend, o->option);
	telnet->opt_states[o->index] = REALLY_INACTIVE;
}

/*
 * Generate side effects of enabling or disabling an option.
 */
static void option_side_effects(Telnet telnet, const struct Opt *o, int enabled) {
	if (o->option == TELOPT_ECHO && o->send == DO)
		telnet->echoing = !enabled;
	else if (o->option == TELOPT_SGA && o->send == DO)
		telnet->editing = !enabled;
	if (telnet->ldisc) /* cause ldisc to notice the change */
		ldisc_send(telnet->ldisc, NULL, 0, 0);

	/* Ensure we get the minimum options */
	if (!telnet->activated) {
		if (telnet->opt_states[o_echo.index] == INACTIVE) {
			telnet->opt_states[o_echo.index] = REQUESTED;
			send_opt(telnet, o_echo.send, o_echo.option);
		}
		if (telnet->opt_states[o_we_sga.index] == INACTIVE) {
			telnet->opt_states[o_we_sga.index] = REQUESTED;
			send_opt(telnet, o_we_sga.send, o_we_sga.option);
		}
		if (telnet->opt_states[o_they_sga.index] == INACTIVE) {
			telnet->opt_states[o_they_sga.index] = REQUESTED;
			send_opt(telnet, o_they_sga.send, o_they_sga.option);
		}
		telnet->activated = TRUE;
	}
}

static void activate_option(Telnet telnet, const struct Opt *o) {
	if (o->send == WILL && o->option == TELOPT_NAWS)
		telnet_size(telnet, telnet->term_width, telnet->term_height);
	if (o->send == WILL
			&& (o->option == TELOPT_NEW_ENVIRON
					|| o->option == TELOPT_OLD_ENVIRON)) {
		/*
		 * We may only have one kind of ENVIRON going at a time.
		 * This is a hack, but who cares.
		 */
		deactivate_option(telnet,
				o->option == TELOPT_NEW_ENVIRON ? &o_oenv : &o_nenv);
	}
	option_side_effects(telnet, o, 1);
}

static void refused_option(Telnet telnet, const struct Opt *o) {
	if (o->send == WILL && o->option == TELOPT_NEW_ENVIRON
			&& telnet->opt_states[o_oenv.index] == INACTIVE) {
		send_opt(telnet, WILL, TELOPT_OLD_ENVIRON);
		telnet->opt_states[o_oenv.index] = REQUESTED;
	}
	option_side_effects(telnet, o, 0);
}

static void proc_rec_opt(Telnet telnet, int cmd, int option) {
	const struct Opt * const *o;

	if ((option == TELOPT_TSPEED || option == TELOPT_NEW_ENVIRON) && cmd == DO) {
		send_opt(telnet, WONT, option);
		return;
	}

	log_option(telnet, "server", cmd, option);
	for (o = opts; *o; o++) {
		if ((*o)->option == option && (*o)->ack == cmd) {
			switch (telnet->opt_states[(*o)->index]) {
			case REQUESTED:
				telnet->opt_states[(*o)->index] = ACTIVE;
				activate_option(telnet, *o);
				break;
			case ACTIVE:
				break;
			case INACTIVE:
				telnet->opt_states[(*o)->index] = ACTIVE;
				send_opt(telnet, (*o)->send, option);
				activate_option(telnet, *o);
				break;
			case REALLY_INACTIVE:
				send_opt(telnet, (*o)->nsend, option);
				break;
			}
			return;
		} else if ((*o)->option == option && (*o)->nak == cmd) {
			switch (telnet->opt_states[(*o)->index]) {
			case REQUESTED:
				telnet->opt_states[(*o)->index] = INACTIVE;
				refused_option(telnet, *o);
				break;
			case ACTIVE:
				telnet->opt_states[(*o)->index] = INACTIVE;
				send_opt(telnet, (*o)->nsend, option);
				option_side_effects(telnet, *o, 0);
				break;
			case INACTIVE:
			case REALLY_INACTIVE:
				break;
			}
			return;
		}
	}
	/*
	 * If we reach here, the option was one we weren't prepared to
	 * cope with. If the request was positive (WILL or DO), we send
	 * a negative ack to indicate refusal. If the request was
	 * negative (WONT / DONT), we must do nothing.
	 */
	if (cmd == WILL || cmd == DO)
		send_opt(telnet, (cmd == WILL ? DONT : WONT), option);
}

static void process_subneg(Telnet telnet) {
	unsigned char b[2048], *p, *q;
	int var, value, n;
	char *e;

	switch (telnet->sb_opt) {
	case TELOPT_TSPEED:
		if (telnet->sb_len == 1 && telnet->sb_buf[0] == TELQUAL_SEND) {
			char *logbuf;
			b[0] = IAC;
			b[1] = SB;
			b[2] = TELOPT_TSPEED;
			b[3] = TELQUAL_IS;
			strcpy((char *) (b + 4), telnet->cfg.termspeed);
			n = 4 + strlen(telnet->cfg.termspeed);
			b[n] = IAC;
			b[n + 1] = SE;
			telnet->bufsize = sk_write(telnet->s, (char * )b, n + 2);
			logevent(telnet->frontend, "server:\tSB TSPEED SEND");
			logbuf = dupprintf("client:\tSB TSPEED IS %s",
					telnet->cfg.termspeed);
			logevent(telnet->frontend, logbuf);
			sfree(logbuf);
		} else
			logevent(telnet->frontend, "server:\tSB TSPEED <something weird>");
		break;
	case TELOPT_TTYPE:
		if (telnet->sb_len == 1 && telnet->sb_buf[0] == TELQUAL_SEND) {
			char *logbuf;
			b[0] = IAC;
			b[1] = SB;
			b[2] = TELOPT_TTYPE;
			b[3] = TELQUAL_IS;
			for (n = 0; telnet->cfg.termtype[n]; n++) {
				b[n + 4] = (
						telnet->cfg.termtype[n] >= 'a'
								&& telnet->cfg.termtype[n] <= 'z' ?
								telnet->cfg.termtype[n] + 'A' - 'a' :
								telnet->cfg.termtype[n]);

				//b[n + 4] = (telnet->cfg.termtype[n] >= 'a'//小写方式
				//    && telnet->cfg.termtype[n] <=
				//    'z' ? telnet->cfg.termtype[n] + 0 -
				//    0 : telnet->cfg.termtype[n]);
			}
			b[n + 4] = IAC;
			b[n + 5] = SE;
			telnet->bufsize = sk_write(telnet->s, (char * )b, n + 6);
			b[n + 4] = 0;
			logevent(telnet->frontend, "server:\tSB TTYPE SEND");
			logbuf = dupprintf("client:\tSB TTYPE IS %s", b + 4);
			logevent(telnet->frontend, logbuf);
			sfree(logbuf);
		} else
			logevent(telnet->frontend,
					"server:\tSB TTYPE <something weird>\r\n");
		break;
	case TELOPT_OLD_ENVIRON:
	case TELOPT_NEW_ENVIRON:
		p = telnet->sb_buf;
		q = p + telnet->sb_len;
		if (p < q && *p == TELQUAL_SEND) {
			char *logbuf;
			p++;
			logbuf = dupprintf("server:\tSB %s SEND", telopt(telnet->sb_opt));
			logevent(telnet->frontend, logbuf);
			sfree(logbuf);
			if (telnet->sb_opt == TELOPT_OLD_ENVIRON) {
				if (telnet->cfg.rfc_environ) {
					value = RFC_VALUE;
					var = RFC_VAR;
				} else {
					value = BSD_VALUE;
					var = BSD_VAR;
				}
				/*
				 * Try to guess the sense of VAR and VALUE.
				 */
				while (p < q) {
					if (*p == RFC_VAR) {
						value = RFC_VALUE;
						var = RFC_VAR;
					} else if (*p == BSD_VAR) {
						value = BSD_VALUE;
						var = BSD_VAR;
					}
					p++;
				}
			} else {
				/*
				 * With NEW_ENVIRON, the sense of VAR and VALUE
				 * isn't in doubt.
				 */
				value = RFC_VALUE;
				var = RFC_VAR;
			}
			b[0] = IAC;
			b[1] = SB;
			b[2] = telnet->sb_opt;
			b[3] = TELQUAL_IS;
			n = 4;
			e = telnet->cfg.environmt;
			while (*e) {
				b[n++] = var;
				while (*e && *e != '\t')
					b[n++] = *e++;
				if (*e == '\t')
					e++;
				b[n++] = value;
				while (*e)
					b[n++] = *e++;
				e++;
			}
			if (*telnet->cfg.username) {
				b[n++] = var;
				b[n++] = 'U';
				b[n++] = 'S';
				b[n++] = 'E';
				b[n++] = 'R';
				b[n++] = value;
				e = telnet->cfg.username;
				while (*e)
					b[n++] = *e++;
			}
			b[n++] = IAC;
			b[n++] = SE;
			telnet->bufsize = sk_write(telnet->s, (char * )b, n);
			logbuf = dupprintf("client:\tSB %s IS %s%s%s%s",
					telopt(telnet->sb_opt),
					*telnet->cfg.username ? "USER=" : "", telnet->cfg.username,
					*telnet->cfg.username ? " " : "",
					n == 6 ?
							"<nothing>" :
							(*telnet->cfg.environmt ? "<stuff>" : ""));
			logevent(telnet->frontend, logbuf);
			sfree(logbuf);
		}
		break;
	}
}
void do_telnet_read(Telnet telnet, byte *buf, int len) {
	char *outbuf = NULL;
	int outbuflen = 0, outbufsize = 0;

#define ADDTOBUF(c) do { \
    if (outbuflen >= outbufsize) { \
	outbufsize = outbuflen + 256; \
        outbuf = sresize(outbuf, outbufsize, char); \
    } \
    outbuf[outbuflen++] = (c); \
} while (0)

	while (len--) {
		int c = (unsigned char) *buf++;

		switch (telnet->state) {
		case TOP_LEVEL:
		case SEENCR:
			if (c == NUL && telnet->state == SEENCR)
				telnet->state = TOP_LEVEL;
			else if (c == IAC)
				telnet->state = SEENIAC;
			else {
				if (!telnet->in_synch)
					ADDTOBUF(c);

#if 1
				/* I can't get the F***ing winsock to insert the urgent IAC
				 * into the right position! Even with SO_OOBINLINE it gives
				 * it to recv too soon. And of course the DM byte (that
				 * arrives in the same packet!) appears several K later!!
				 *
				 * Oh well, we do get the DM in the right place so I'll
				 * just stop hiding on the next 0xf2 and hope for the best.
				 */
				else if (c == DM)
					telnet->in_synch = 0;
#endif
				if (c == CR && telnet->opt_states[o_they_bin.index] != ACTIVE)
					telnet->state = SEENCR;
				else
					telnet->state = TOP_LEVEL;
			}
			break;
		case SEENIAC:
			if (c == DO)
				telnet->state = SEENDO;
			else if (c == DONT)
				telnet->state = SEENDONT;
			else if (c == WILL)
				telnet->state = SEENWILL;
			else if (c == WONT)
				telnet->state = SEENWONT;
			else if (c == SB)
				telnet->state = SEENSB;
			else if (c == DM) {
				telnet->in_synch = 0;
				telnet->state = TOP_LEVEL;
			}

			else {
				/* ignore everything else; print it if it's IAC */
				if (c == IAC) {
					ADDTOBUF(c);
				}
				telnet->state = TOP_LEVEL;
			}
			break;
		case SEENWILL:
			proc_rec_opt(telnet, WILL, c);
			telnet->state = TOP_LEVEL;
			break;
		case SEENWONT:
			proc_rec_opt(telnet, WONT, c);
			telnet->state = TOP_LEVEL;
			break;
		case SEENDO:
			proc_rec_opt(telnet, DO, c);
			telnet->state = TOP_LEVEL;
			break;
		case SEENDONT:
			proc_rec_opt(telnet, DONT, c);
			telnet->state = TOP_LEVEL;
			break;
		case SEENSB:
			telnet->sb_opt = c;
			telnet->sb_len = 0;
			telnet->state = SUBNEGOT;
			break;
		case SUBNEGOT:
			if (c == IAC) {
				telnet->state = SUBNEG_IAC;
			} else {
				subneg_addchar: if (telnet->sb_len >= telnet->sb_size) {
					telnet->sb_size += SB_DELTA;
					telnet->sb_buf = sresize(telnet->sb_buf, telnet->sb_size,
							unsigned char);
				}
				telnet->sb_buf[telnet->sb_len++] = c;
				telnet->state = SUBNEGOT; /* in case we came here by goto */
			}
			break;
		case SUBNEG_IAC:
			if (c != SE)
				goto subneg_addchar;
			/* yes, it's a hack, I know, but... */
			else {
				process_subneg(telnet);
				telnet->state = TOP_LEVEL;
			}
			break;
		}
	}
	if (outbuflen) {
		c_write(telnet, outbuf, outbuflen);
		{
			autologinpass(outbuf, outbuflen);
		}
	}

	sfree(outbuf);
}

static void telnet_log(Plug plug, int type, SockAddr addr, int port,
		const char *error_msg, int error_code) {
	Telnet telnet = (Telnet) plug;
	char addrbuf[256], *msg;

	sk_getaddr(addr, addrbuf, lenof(addrbuf));

	if (type == 0)
		msg = dupprintf("Connecting to %s port %d", addrbuf, port);
	else
		msg = dupprintf("Failed to connect to %s: %s", addrbuf, error_msg);

	logevent(telnet->frontend, msg);
}

//先收到这里，然后再调用notify_remote_exit,再给窗口发送消息
static int telnet_closing(Plug plug, const char *error_msg, int error_code,
		int calling_back) {
	Telnet telnet = (Telnet) plug;
	if (telnet->s) {
		sk_close(telnet->s);
		telnet->s = NULL;
		notify_remote_exit(telnet->frontend);
	}
	if (error_msg) {
		logevent(telnet->frontend, error_msg);
		connection_fatal(telnet->frontend, "%s", error_msg);
	}
	/* Otherwise, the remote side closed the connection normally. */
	return 0;
}

static int telnet_receive(Plug plug, int urgent, byte *data, int len) {
	Telnet telnet = (Telnet) plug;
	if (urgent)
		telnet->in_synch = TRUE;

	//zhangbo
	log_write(SOCK_RECV, data, len);

	const unsigned char init11[] = { 0xff, 0xf1, 0x18, 0x00, 0x00, 0x00 };	//实达
	const unsigned char init21[] = { 0xff, 0xfd, 0x18, 0xff, 0xfd, 0x7d };	//国光
	const unsigned char init22[] = { 0xff, 0xfb, 0x18, 0xff, 0xfb, 0x7d };	//国光
	const unsigned char init23[] = { 0xFF, 0xFA, 0x18, 0x01, 0xFF, 0xF0, 0xFF,
			0xFA, 0x7D, 0x01, 0xFF, 0xF0 };				//国光
	unsigned char init24[] = { 0xFF, 0xFA, 0x18, 0x00, 0x56, 0x54, 0x31,
			0x30, 0x30, 0xFF, 0xF0, 0xFF, 0xFA, 0x7D, 0x00, 0x43, 0x4A,
			0x36, 0x38, 0x30, 0x30, 0x42, 0x2C, 0x61, 0x6E, 0x79, 0x2C };
	unsigned char init25[] = { 0xFF, 0xF0 };
	if (len >= 4 && memcmp(data, init11, 4) == 0) {
		char screennum[135];
		sprintf(screennum, "\xFF\xF1%s", cfg.screennum);
		sk_write(telnet->s, (char * )screennum, strlen(screennum));
		return 0;
	} else if (len >= 6 && memcmp(data, init21, 6) == 0) {
		sk_write(telnet->s, (char * )init22, 6);
		return 0;
	} else if (len >= 12 && memcmp(data, init23, 12) == 0) {
		unsigned char screennum[135];
		int l = strlen(cfg.screennum);
		memcpy(screennum, init24, 27);
		memcpy(screennum+27, cfg.screennum, l);
		memcpy(screennum+27+l, init25, 2);
		sk_write(telnet->s, (char * )screennum, 29 + l);
		return 0;
	}

//	if (cfg.enablescreennum) {
//		static int state = 0;
//		if (state == 0) {
//			const unsigned char init1[] = { 0xFF, 0xF1, 0x18, 0x00, 0x00, 0x00 };
//			const unsigned char init2[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//			if (memcmp(data, init1, 6) == 0) {
//				char screennum[4];
//				sprintf(screennum, "\xFF\xF1%02d", cfg.screennum);
//				sk_write(telnet->s, (char *)screennum, 4);
//			}
//			state++;
//			return 0;
//		} else if (state == 1) {
//			const struct Opt * const *o;
//			for (o = opts; *o; o++)
//				telnet->opt_states[(*o)->index] = INACTIVE;
////			telnet->activated = TRUE;
//
//
////			if (telnet->cfg.passive_telnet) {
////				const struct Opt * const *o;
////				for (o = opts; *o; o++)
////					telnet->opt_states[(*o)->index] = INACTIVE;
////			} else {
////				const struct Opt * const *o;
////
////				for (o = opts; *o; o++) {
////					telnet->opt_states[(*o)->index] = (*o)->initial_state;
////					if (telnet->opt_states[(*o)->index] == REQUESTED) {
////						send_opt(telnet, (*o)->send, (*o)->option);
////					}
////				}
////				telnet->activated = TRUE;
////			}
//			state++;
//		}
//	}

	do_telnet_read(telnet, data, len);

	return 0;
}

static void telnet_sent(Plug plug, int bufsize) {
	Telnet telnet = (Telnet) plug;
	telnet->bufsize = bufsize;
}

/*
 * Called to set up the Telnet connection.
 *
 * Returns an error message, or NULL on success.
 *
 * Also places the canonical host name into `realhost'. It must be
 * freed by the caller.
 */
static const char *telnet_init(void *frontend_handle, void **backend_handle,
		Config *cfg, char *host, int port, char **realhost, int nodelay,
		int keepalive) {
	static const struct plug_function_table fn_table = { telnet_log,
			telnet_closing, telnet_receive, telnet_sent };
	SockAddr addr;
	const char *err;
	Telnet telnet;
	//char strChar[128];

	telnet = snew(struct telnet_tag);
	telnet->fn = &fn_table;
	telnet->cfg = *cfg; /* STRUCTURE COPY */
	telnet->s = NULL;
	telnet->echoing = TRUE;
	telnet->editing = TRUE;
	telnet->activated = FALSE;
	telnet->sb_buf = NULL;
	telnet->sb_size = 0;
	telnet->frontend = frontend_handle;
	telnet->term_width = telnet->cfg.width;
	telnet->term_height = telnet->cfg.height;
	telnet->state = TOP_LEVEL;
	telnet->ldisc = NULL;
	telnet->pinger = NULL;
	*backend_handle = telnet;

	/*
	 * Try to find host.
	 */
	{
		char *buf;
		buf =
				dupprintf("Looking up host \"%s\"%s", host,
						(cfg->addressfamily == ADDRTYPE_IPV4 ?
								" (IPv4)" :
								(cfg->addressfamily == ADDRTYPE_IPV6 ?
										" (IPv6)" : "")));
		logevent(telnet->frontend, buf);
		sfree(buf);
	}
	addr = name_lookup(host, port, realhost, &telnet->cfg, cfg->addressfamily);
	if ((err = sk_addr_error(addr)) != NULL) {
		sk_addr_free(addr);
		return err;
	}

	if (port < 0)
		port = 23; /* default telnet port */

	/*
	 * Open socket.
	 */
	telnet->s = new_connection(addr, *realhost, port, 0, 1, nodelay, keepalive,
			(Plug) telnet, &telnet->cfg);

	if ((err = sk_socket_error(telnet->s)) != NULL)
		return err;

	telnet->pinger = pinger_new(&telnet->cfg, &telnet_backend, telnet);

	const struct Opt * const *o;
	for (o = opts; *o; o++)
		telnet->opt_states[(*o)->index] = INACTIVE;

	/*
	 * Set up SYNCH state.
	 */
	telnet->in_synch = FALSE;

	/*
	 * We can send special commands from the start.
	 */
	update_specials_menu(telnet->frontend);

	return NULL;
}

static void telnet_free(void *handle) {
	Telnet telnet = (Telnet) handle;

	sfree(telnet->sb_buf);
	if (telnet->s)
		sk_close(telnet->s);
	if (telnet->pinger)
		pinger_free(telnet->pinger);
	sfree(telnet);
}
/*
 * Reconfigure the Telnet backend. There's no immediate action
 * necessary, in this backend: we just save the fresh config for
 * any subsequent negotiations.
 */
static void telnet_reconfig(void *handle, Config *cfg) {
	Telnet telnet = (Telnet) handle;
	pinger_reconfig(telnet->pinger, &telnet->cfg, cfg);
	telnet->cfg = *cfg; /* STRUCTURE COPY */
}

/*
 * Called to send data down the Telnet connection.
 */

static int telnet_send(void *handle, char *buf, int len) {
	Telnet telnet = (Telnet) handle;
	unsigned char *p, *end;
	static const unsigned char iac[2] = { IAC, IAC };
	static const unsigned char cr[2] = { CR, NUL };

#if 0
	static const unsigned char nl[2] = {CR, LF};
#endif

	if (telnet->s == NULL)
		return 0;

	p = (unsigned char *) buf;
	end = (unsigned char *) (buf + len);
	while (p < end) {
		unsigned char *q = p;

		while (p < end && iswritable(*p)) {
			p++;
		}
		telnet->bufsize = sk_write(telnet->s, (char * )q, p - q);

		while (p < end && !iswritable(*p)) {
			telnet->bufsize = sk_write(telnet->s,
					(char *)(*p == IAC ? iac : cr), 2);
			p++;
		}
	}

	return telnet->bufsize;
}

static int telnet_send_BIN(void *handle, byte *buf, int len) {
	Telnet telnet = (Telnet) handle;
	telnet->bufsize = sk_write(telnet->s, (const char * )buf, len);
	return telnet->bufsize;
}

/*
 * Called to query the current socket sendability status.
 */
static int telnet_sendbuffer(void *handle) {
	Telnet telnet = (Telnet) handle;
	return telnet->bufsize;
}

/*
 * Called to set the size of the window from Telnet's POV.
 */
static void telnet_size(void *handle, int width, int height) {
	Telnet telnet = (Telnet) handle;
	unsigned char b[24];
	int n;
	char *logbuf;

	telnet->term_width = width;
	telnet->term_height = height;

	if (telnet->s == NULL || telnet->opt_states[o_naws.index] != ACTIVE)
		return;
	n = 0;
	b[n++] = IAC;
	b[n++] = SB;
	b[n++] = TELOPT_NAWS;
	b[n++] = telnet->term_width >> 8;
	if (b[n - 1] == IAC)
		b[n++] = IAC; /* duplicate any IAC byte occurs */
	b[n++] = telnet->term_width & 0xFF;
	if (b[n - 1] == IAC)
		b[n++] = IAC; /* duplicate any IAC byte occurs */
	b[n++] = telnet->term_height >> 8;
	if (b[n - 1] == IAC)
		b[n++] = IAC; /* duplicate any IAC byte occurs */
	b[n++] = telnet->term_height & 0xFF;
	if (b[n - 1] == IAC)
		b[n++] = IAC; /* duplicate any IAC byte occurs */
	b[n++] = IAC;
	b[n++] = SE;
	telnet->bufsize = sk_write(telnet->s, (char * )b, n);
	logbuf = dupprintf("client:\tSB NAWS %d,%d", telnet->term_width,
			telnet->term_height);
	logevent(telnet->frontend, logbuf);
	sfree(logbuf);
}

/*
 * Send Telnet special codes.
 */
static void telnet_special(void *handle, Telnet_Special code) {
	Telnet telnet = (Telnet) handle;
	unsigned char b[2];

	if (telnet->s == NULL)
		return;

	log_write_err(SYS_INFO, "telnet_special", code);

	b[0] = IAC;
	switch (code) {
	case TS_AYT:
		b[1] = AYT; //表示对方是否还在运行
		telnet->bufsize = sk_write(telnet->s, (char * )b, 2);
		break;
	case TS_BRK:
		b[1] = BREAK;
		telnet->bufsize = sk_write(telnet->s, (char * )b, 2);
		break;
	case TS_EC:
		b[1] = EC;
		telnet->bufsize = sk_write(telnet->s, (char * )b, 2);
		break;
	case TS_EL:
		b[1] = EL;
		telnet->bufsize = sk_write(telnet->s, (char * )b, 2);
		break;
	case TS_GA:
		b[1] = GA;
		telnet->bufsize = sk_write(telnet->s, (char * )b, 2);
		break;
	case TS_NOP:
		b[1] = NOP;
		telnet->bufsize = sk_write(telnet->s, (char * )b, 2);
		break;
	case TS_ABORT:
		b[1] = ABORT;
		telnet->bufsize = sk_write(telnet->s, (char * )b, 2);
		break;
	case TS_AO:
		b[1] = AO;
		telnet->bufsize = sk_write(telnet->s, (char * )b, 2);
		break;
	case TS_IP:
		b[1] = IP;
		telnet->bufsize = sk_write(telnet->s, (char * )b, 2);
		break;
	case TS_SUSP:
		b[1] = SUSP; //挂起当前的进程
		telnet->bufsize = sk_write(telnet->s, (char * )b, 2);
		break;
	case TS_EOR:
		b[1] = EOR; //记录结束符
		telnet->bufsize = sk_write(telnet->s, (char * )b, 2);
		break;
	case TS_EOF:
		b[1] = xEOF;
		telnet->bufsize = sk_write(telnet->s, (char * )b, 2);
		break;
	case TS_EOL:
		/* In BINARY mode, CR-LF becomes just CR -
		 * and without the NUL suffix too. */
		if (telnet->opt_states[o_we_bin.index] == ACTIVE)
			telnet->bufsize = sk_write(telnet->s, "\r", 1);
		else
			telnet->bufsize = sk_write(telnet->s, "\r\n", 2);
		break;
	case TS_SYNCH:
		b[1] = DM;
		telnet->bufsize = sk_write(telnet->s, (char * )b, 1);
		telnet->bufsize = sk_write_oob(telnet->s, (char * )(b + 1), 1);
		break;
	case TS_RECHO:
		if (telnet->opt_states[o_echo.index] == INACTIVE
				|| telnet->opt_states[o_echo.index] == REALLY_INACTIVE) {
			telnet->opt_states[o_echo.index] = REQUESTED;
			send_opt(telnet, o_echo.send, o_echo.option);
		}
		break;
	case TS_LECHO:
		if (telnet->opt_states[o_echo.index] == ACTIVE) {
			telnet->opt_states[o_echo.index] = REQUESTED;
			send_opt(telnet, o_echo.nsend, o_echo.option);
		}
		break;
	case TS_PING:
		if (telnet->opt_states[o_they_sga.index] == ACTIVE) {
			b[1] = NOP;
			telnet->bufsize = sk_write(telnet->s, (char * )b, 2);
		}
		break;
	default:
		break; /* never heard of it */
	}
}

static const struct telnet_special *telnet_get_specials(void *handle) {
	static const struct telnet_special specials[] = {
			{ "Are You There", TS_AYT }, { "Break", TS_BRK }, { "Synch",
					TS_SYNCH }, { "Erase Character", TS_EC }, { "Erase Line",
					TS_EL }, { "Go Ahead", TS_GA }, { "No Operation", TS_NOP },
			{ NULL, TS_SEP }, { "Abort Process", TS_ABORT }, { "Abort Output",
					TS_AO }, { "Interrupt Process", TS_IP }, {
					"Suspend Process", TS_SUSP }, { NULL, TS_SEP }, {
					"End Of Record", TS_EOR }, { "End Of File", TS_EOF }, {
			NULL, TS_EXITMENU } };
	return specials;
}

static int telnet_connected(void *handle) {
	Telnet telnet = (Telnet) handle;
	return telnet->s != NULL;
}

static int telnet_sendok(void *handle) {
	/* Telnet telnet = (Telnet) handle; */
	return 1;
}

static void telnet_unthrottle(void *handle, int backlog) {
	Telnet telnet = (Telnet) handle;
	sk_set_frozen(telnet->s, backlog > TELNET_MAX_BACKLOG);
}

static int telnet_ldisc(void *handle, int option) {
	Telnet telnet = (Telnet) handle;
	if (option == LD_ECHO)
		return telnet->echoing;
	if (option == LD_EDIT)
		return telnet->editing;
	return FALSE;
}

static void telnet_provide_ldisc(void *handle, void *ldisc) {
	Telnet telnet = (Telnet) handle;
	telnet->ldisc = ldisc;
}

static void telnet_provide_logctx(void *handle, void *logctx) {
	/* This is a stub. */
}

static int telnet_exitcode(void *handle) {
	Telnet telnet = (Telnet) handle;
	if (telnet->s != NULL)
		return -1; /* still connected */
	else
		/* Telnet doesn't transmit exit codes back to the client */
		return 0;
}

/*
 * cfg_info for Telnet does nothing at all.
 */
static int telnet_cfg_info(void *handle) {
	return 0;
}

Backend telnet_backend = { telnet_init, telnet_free, telnet_reconfig,
		telnet_send, telnet_send_BIN, telnet_sendbuffer, telnet_size,
		telnet_special, telnet_get_specials, telnet_connected, telnet_exitcode,
		telnet_sendok, telnet_ldisc, telnet_provide_ldisc,
		telnet_provide_logctx, telnet_unthrottle, telnet_cfg_info, 23 };
