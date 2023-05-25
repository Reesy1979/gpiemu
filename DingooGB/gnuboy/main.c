#undef _GNU_SOURCE
#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdarg.h>
#include <signal.h>

#include "input.h"
#include "rc.h"
#include "sys.h"
#include "rckeys.h"
#include "emu.h"
#include "exports.h"
#include "loader.h"
#include "mem.h"
#include "menu.h"

#include "Version"

#ifdef __psp__
#include <pspmoduleinfo.h>
#include <pspthreadman.h>
PSP_MODULE_INFO("GnuBoy", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_VFPU | THREAD_ATTR_USER);
#endif


static char *defaultconfig[] =
{
	"bootrom_dmg \"\"",
	"bootrom_gbc \"\"",
	"bind esc quit",
	"bind up +up",
	"bind down +down",
	"bind left +left",
	"bind right +right",
	"bind d +a",
	"bind s +b",
	"bind enter +start",
	"bind space +select",
	"bind tab +select",
	"bind joyup +up",
	"bind joydown +down",
	"bind joyleft +left",
	"bind joyright +right",
	"bind joy0 +b",
	"bind joy1 +a",
	"bind joy2 +select",
	"bind joy3 +start",
	"bind 1 \"set saveslot 1\"",
	"bind 2 \"set saveslot 2\"",
	"bind 3 \"set saveslot 3\"",
	"bind 4 \"set saveslot 4\"",
	"bind 5 \"set saveslot 5\"",
	"bind 6 \"set saveslot 6\"",
	"bind 7 \"set saveslot 7\"",
	"bind 8 \"set saveslot 8\"",
	"bind 9 \"set saveslot 9\"",
	"bind 0 \"set saveslot 0\"",
	"bind ins savestate",
	"bind del loadstate",
	"set romdir .",
	"source gnuboy.rc",
	NULL
};


static void banner()
{
	printf("\ngnuboy " VERSION "\n");
}

static void copyright()
{
	banner();
	printf(
"Copyright (C) 2000-2001 Laguna and Gilgamesh\n"
"Portions contributed by other authors; see CREDITS for details.\n"
"\n"
"This program is free software; you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation; either version 2 of the License, or\n"
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program; if not, write to the Free Software\n"
"Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n"
"\n");
}

static void usage(char *name)
{
	copyright();
	printf("Type %s --help for detailed help.\n\n", name);
	exit(1);
}

static void copying()
{
	copyright();
	exit(0);
}

#if !defined(_GPI) && !defined(_LINUX)
static void joytest()
{
	event_t e;
	char *ename, *kname;
	printf("press joystick buttons to see their name mappings\n");
	joy_init();
	while(1) {
		ev_poll(0);
		if(ev_getevent(&e)) {
			switch(e.type) {
			case EV_NONE: ename = "none"; break;
			case EV_PRESS: ename = "press"; break;
			case EV_RELEASE: ename = "release"; break;
			case EV_REPEAT: ename = "repeat"; break;
			case EV_MOUSE: ename = "mouse"; break;
			default: ename = "unknown";
			};
			kname = k_keyname(e.code);
			printf("%s: %s\n", ename, kname ? kname : "<null>");
		} else {
			sys_sleep(300);
		}
	}
	joy_close();
	exit(0);
}
#endif

static void help(char *name)
{
	banner();
	printf("Usage: %s [options] romfile\n", name);
	printf("\n"
"      --source FILE             read rc commands from FILE\n"
"      --bind KEY COMMAND        bind KEY to perform COMMAND\n"
"      --VAR=VALUE               set rc variable VAR to VALUE\n"
"      --VAR                     set VAR to 1 (turn on boolean options)\n"
"      --no-VAR                  set VAR to 0 (turn off boolean options)\n"
"      --showvars                list all available rc variables\n"
"      --help                    display this help and exit\n"
"      --version                 output version information and exit\n"
"      --copying                 show copying permissions\n"
"      --joytest                 init joystick and show button names pressed\n"
"      --rominfo                 show some info about the rom\n"
"");
	exit(0);
}

static void rominfo(char* fn) {
	extern int rom_load_simple(char *fn);
	if(rom_load_simple(fn)) {
		fprintf(stderr, "rom load failed: %s\n", loader_get_error()?loader_get_error():"");
		exit(1);
	}
	printf( "rom name:\t%s\n"
		"mbc type:\t%s\n"
		"rom size:\t%u KB\n"
		"ram size:\t%u KB\n"
		, rom.name, mbc_to_string(mbc.type), mbc.romsize*16, mbc.ramsize*8);
	exit(0);
}

static void version(char *name)
{
	printf("%s-" VERSION "\n", name);
	exit(0);
}


void doevents()
{
#if !defined(_GPI) && !defined(_LINUX)
	event_t ev;
	int st;

	ev_poll(0);
	while (ev_getevent(&ev))
	{
		if (ev.type != EV_PRESS && ev.type != EV_RELEASE)
			continue;
		st = (ev.type != EV_RELEASE);
		rc_dokey(ev.code, st);
	}
#endif
}



#if !defined(_GPI) && !defined(_LINUX)
static void shutdown()
{
	joy_close();
	vid_close();
	pcm_close();
}
#endif

void die(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(1);
}

static int bad_signals[] =
{
	/* These are all standard, so no need to #ifdef them... */
	SIGINT, SIGSEGV, SIGTERM, SIGFPE, SIGABRT, SIGILL,
#ifdef SIGQUIT
	SIGQUIT,
#endif
#ifdef SIGPIPE
	SIGPIPE,
#endif
	0
};

static void fatalsignal(int s)
{
	die("Signal %d\n", s);
}

static void catch_signals()
{
	int i;
	for (i = 0; bad_signals[i]; i++)
		signal(bad_signals[i], fatalsignal);
}



static char *base(char *s)
{
	char *p;
	p = strrchr(s, '/');
	if (p) return p+1;
	return s;
}

int load_rom_and_rc(char *rom) {
	char *s, *cmd = malloc(strlen(rom) + 11);
	sprintf(cmd, "source %s", rom);
	s = strchr(cmd, '.');
	if (s) *s = 0;
	strcat(cmd, ".rc");
	rc_command(cmd);
	free(cmd);
	rom = strdup(rom);
	sys_sanitize(rom);
	if(loader_init(rom)) {
		/*loader_get_error();*/
		return -1;
	}
	emu_reset();
	return 0;
}

#if !defined(_GPI) && !defined(_LINUX)
int main(int argc, char *argv[])
{
	int i, ri = 0, sv = 0;
	char *opt, *arg, *cmd, *s, *rom = 0;

	/* Avoid initializing video if we don't have to */
	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--help"))
			help(base(argv[0]));
		else if (!strcmp(argv[i], "--version"))
			version(base(argv[0]));
		else if (!strcmp(argv[i], "--copying"))
			copying();
		else if (!strcmp(argv[i], "--bind")) i += 2;
		else if (!strcmp(argv[i], "--source")) i++;
		else if (!strcmp(argv[i], "--showvars")) sv = 1;
		else if (!strcmp(argv[i], "--joytest"))
			joytest();
		else if (!strcmp(argv[i], "--rominfo")) ri = 1;
		else if (argv[i][0] == '-' && argv[i][1] == '-');
		else if (argv[i][0] == '-' && argv[i][1]);
		else rom = argv[i];
	}

	if (ri && !rom) usage(base(argv[0]));
	if (ri) rominfo(rom);

	/* If we have special perms, drop them ASAP! */
	vid_preinit();

	init_exports();

	s = strdup(argv[0]);
	sys_sanitize(s);
	sys_initpath(s);

	for (i = 0; defaultconfig[i]; i++)
		rc_command(defaultconfig[i]);

	if (sv) {
		show_exports();
		exit(0);
	}

	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--bind"))
		{
			if (i + 2 >= argc) die("missing arguments to bind\n");
			cmd = malloc(strlen(argv[i+1]) + strlen(argv[i+2]) + 9);
			sprintf(cmd, "bind %s \"%s\"", argv[i+1], argv[i+2]);
			rc_command(cmd);
			free(cmd);
			i += 2;
		}
		else if (!strcmp(argv[i], "--source"))
		{
			if (i + 1 >= argc) die("missing argument to source\n");
			cmd = malloc(strlen(argv[i+1]) + 6);
			sprintf(cmd, "source %s", argv[++i]);
			rc_command(cmd);
			free(cmd);
		}
		else if (!strncmp(argv[i], "--no-", 5))
		{
			opt = strdup(argv[i]+5);
			while ((s = strchr(opt, '-'))) *s = '_';
			cmd = malloc(strlen(opt) + 7);
			sprintf(cmd, "set %s 0", opt);
			rc_command(cmd);
			free(cmd);
			free(opt);
		}
		else if (argv[i][0] == '-' && argv[i][1] == '-')
		{
			opt = strdup(argv[i]+2);
			if ((s = strchr(opt, '=')))
			{
				*s = 0;
				arg = s+1;
			}
			else arg = "1";
			while ((s = strchr(opt, '-'))) *s = '_';
			while ((s = strchr(arg, ','))) *s = ' ';
			
			cmd = malloc(strlen(opt) + strlen(arg) + 6);
			sprintf(cmd, "set %s %s", opt, arg);
			
			rc_command(cmd);
			free(cmd);
			free(opt);
		}
		/* short options not yet implemented */
		else if (argv[i][0] == '-' && argv[i][1]);
	}

	/* FIXME - make interface modules responsible for atexit() */
	atexit(shutdown);
	catch_signals();
	vid_init();
	joy_init();
	pcm_init();
	menu_init();

	if(rom) load_rom_and_rc(rom);
	else {
		rc_command("bind esc menu");
		menu_initpage(mp_romsel);
		menu_enter();
	}
	while(1) {
		emu_run();
		/* if we get here it means emu was paused, so enter menu. */
		pcm_pause(1);
		menu_initpage(mp_main);
		menu_enter();
		pcm_pause(0);
		emu_pause(0);
	}

	/* never reached */
	return 0;
}

#endif










