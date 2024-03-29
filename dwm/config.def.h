#include "tcl.c"
#include "movestack.c"
#include "fibonacci.c"
#include <X11/XF86keysym.h>

/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx  = 3;        /* border pixel of windows */
static const unsigned int gappx     = 0;        /* gaps between windows */
static const unsigned int snap      = 32;       /* snap pixel */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayonleft = 0;    /* 0: systray in the right corner, >0: systray on left of status text */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const int showsystray        = 1;        /* 0 means no systray */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */
static const double defaultopacity  = 1.00;
static const char *fonts[]          = { "JetBrainsMono Nerd Font:style=SemiBold:size=12.0" };
static const char dmenufont[]       = "JetBrainsMono Nerd Font:style=SemiBold:size=12.0";
static const char col_dark[]        = "#111111";
static const char col_darkergray[]  = "#333333";
static const char col_darkgray[]    = "#E0E0E0";
static const char col_lightgray[]   = "#bbbbbb";
static const char col_white[]       = "#ffffff";
static const char col_cyan[]        = "#005577";
static const char col_fedora[]      = "#072c61";
static const char col_purple[]      = "#4d3573";
static const unsigned int baralpha    = 0xff; // originally set to '0xd0'
static const unsigned int borderalpha = OPAQUE;
static const char *colors[][3]      = {
	/*               fg         bg         border   */
	[SchemeNorm] = { col_white, col_dark, col_darkergray },
	[SchemeSel]  = { col_white, col_purple, col_purple },
};
static const unsigned int alphas[][3]      = {
	/*               fg      bg        border     */
	[SchemeNorm] = { OPAQUE, baralpha, borderalpha },
	[SchemeSel]  = { OPAQUE, baralpha, borderalpha },
};

/* tagging */
static const char *tags[] = { "", "󰈹", "", "󰨞", "", "󰙯", "", "󰇅", "" };

static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class      instance    title       tags mask     isfloating   monitor */
	{ "Gimp",     NULL,       NULL,       0,            1,           -1 },
	{ "Firefox",  NULL,       NULL,       1 << 8,       0,           -1 },
};

/* layout(s) */
static const float mfact     = 0.55;   /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;      /* number of clients in master area */
static const int resizehints = 1;      /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1;   /* 1 will force focus on the fullscreen window */
static const int attachdirection = 4;  /* 0 default, 1 above, 2 aside, 3 below, 4 bottom, 5 top */


static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "",      tile },    /* first entry is default */
	{ "",      NULL },    /* no layout function means floating behavior */
	{ "",      monocle },
	{ "󱇜",      tcl },
	{ "",      spiral },
	{ "󰁃",      dwindle },
	{ "󱥢",     centeredmaster },
	{ "󰼀",    centeredfloatingmaster },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/usr/local/bin/st", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[]      = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", col_dark, "-nf", col_lightgray, "-sb", col_purple, "-sf", col_white, NULL };
static const char *termcmd[]       = { "kitty", NULL };
static const char *sttermcmd[]     = { "st", NULL};
static const char *lockcmd[]       = { "slock", NULL };
static const char *filemgrcmd[]    = { "dolphin", NULL };
static const char *mutecmd[]       = {"/usr/bin/pactl", "set-sink-mute", "@DEFAULT_SINK@", "toggle", NULL};
static const char *mediapausecmd[] = {"/usr/bin/playerctl", "play-pause", NULL};
static const char *mediaprevcmd[]  = {"/usr/bin/playerctl", "previous", NULL};
static const char *medianextcmd[]  = {"/usr/bin/playerctl", "next", NULL};
static const char *audioraisecmd[] = {"wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", "5%+", "-l", "1.0", NULL};
static const char *audiolowercmd[] = {"wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", "5%-", NULL};
static const char *brightraisecmd[]= {"brightnessctl", "s", "+4", NULL};
static const char *brightlowercmd[]= {"brightnessctl", "s", "4-", NULL};

static const Key keys[] = {
	/* modifier                     key        function        argument */
	{ MODKEY,                       XK_p,      spawn,          {.v = dmenucmd } },
	{ MODKEY,                       XK_Return, spawn,          {.v = termcmd } },
	{ MODKEY|ShiftMask,             XK_Return, spawn,          {.v = sttermcmd } },
	{ MODKEY,                       XK_Escape, spawn,          {.v = lockcmd } },
        { MODKEY,                       XK_slash,  spawn,          {.v = filemgrcmd } },
	{ MODKEY,                       XK_b,      togglebar,      {0} },
	{ MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_j,      movestack,      {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_k,      movestack,      {.i = -1 } },
	{ MODKEY,                       XK_i,      incnmaster,     {.i = +1 } },
	{ MODKEY,                       XK_d,      incnmaster,     {.i = -1 } },
	{ MODKEY,                       XK_bracketleft,  setmfact, {.f = -0.05} },
	{ MODKEY,                       XK_bracketright, setmfact, {.f = +0.05} },
	{ MODKEY|ShiftMask,             XK_bracketright, setcfact, {.f = +0.25} },
	{ MODKEY|ShiftMask,             XK_bracketleft,  setcfact, {.f = -0.25} },
	{ MODKEY,                       XK_grave,  zoom,           {0} },
	{ MODKEY,                       XK_Tab,    view,           {0} },
	{ MODKEY,                       XK_q,      killclient,     {0} },
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
	{ MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
	{ MODKEY,                       XK_w,      setlayout,      {.v = &layouts[3]} },
	{ MODKEY,                       XK_r,      setlayout,      {.v = &layouts[4]} },
	{ MODKEY|ShiftMask,             XK_r,      setlayout,      {.v = &layouts[5]} },
	{ MODKEY,                       XK_u,      setlayout,      {.v = &layouts[6]} },
	{ MODKEY,                       XK_o,      setlayout,      {.v = &layouts[7]} },
	{ MODKEY,                       XK_space,  setlayout,      {0} },
	{ MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
	{ MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_period, focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
        { 0,                            XF86XK_AudioMute,  spawn,          {.v = mutecmd} },
        { 0,                            XF86XK_AudioPlay,  spawn,          {.v = mediapausecmd } },
        { 0,                            XF86XK_AudioPrev,  spawn,          {.v = mediaprevcmd} },
        { 0,                            XF86XK_AudioNext,  spawn,          {.v = medianextcmd} },
        { 0,                            XF86XK_AudioRaiseVolume,   spawn,  {.v = audioraisecmd} },
        { 0,                            XF86XK_AudioLowerVolume,   spawn,  {.v = audiolowercmd} },
	{ 0,                            XF86XK_MonBrightnessUp,    spawn,  {.v = brightraisecmd} },
	{ 0,                            XF86XK_MonBrightnessDown,  spawn,  {.v = brightlowercmd} },
	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
	{ MODKEY|ShiftMask,             XK_q,      quit,           {0} },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

