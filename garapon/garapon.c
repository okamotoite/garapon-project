/*  garapon - ncurses based lottery game
    Copyright (C) 2020 Junji Okamoto

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>. */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <err.h>
#include <limits.h>
#include <menu.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "garapon.h"

#define ENTER 10
#define NOT_SET 0

void finish(int status);

char *choices[] = {
	"mini garapon", "garapon six", "garapon seven",
	"power garapon", "mega garapon", "super garapon",
	"garapon help", "garapon quit", (char *) NULL
};

vector
newvec(size_t n)
{
	return (int *) calloc(n, sizeof(int));
}

vector
new_vector(size_t n)
{
	vector v;

	if ((v = newvec(n)) == NULL)
		err(1, NULL);
	return v;
}

void
free_vector(vector v)
{
	free(v);
}

struct point
makepoint(int x, int y)
{
	struct point temp;

	temp.x = x;
	temp.y = y;
	return temp;
}

GARAPON *
new_machine(void)
{
	GARAPON *p;

	if ((p = (GARAPON *) malloc(sizeof(GARAPON))) == NULL)
		return NULL;
	bzero((void *) p, sizeof(GARAPON));
	return p;
}

GARAPON *
make_machine(size_t n, int num, int sample, int bonus, chtype color)
{
	GARAPON *p;

	if ((int) n < num || num < sample || sample < bonus || bonus < 0)
		return NULL;
	if ((p = new_machine()) == NULL)
		return NULL;
	p->size = n;
	p->number = num;
	p->sample = sample;
	p->omake = bonus;
	p->color = color;
	p->v = new_vector(n);
	return p;
}

void
free_machine(GARAPON *a)
{
	free_vector(a->v);
	free(a);
}

double
rnd(void)
{
	return (1.0 / (RAND_MAX + 1.0) * (double) random());
}

static int
colorful(WINDOW *win, const int n)
{
	if (win == NULL)
		win = stdscr;
	if (n == 0)
		wattrset(win, COLOR_PAIR(10));
	else
		wattrset(win, COLOR_PAIR(n % 7 + 1));
	return n;
}

static void
printvec(WINDOW *win, struct point *start, int newline, GARAPON *machine)
{
	int i, x, y;

	if (win == NULL)
		win = stdscr;
	x = start->x;
	y = start->y;
	if (machine->color == NOT_SET) {
		for (i = 0; i < (int) machine->size; ++i) {
			mvwprintw(win,
			    y, x, "%02d", colorful(win, machine->v[i]));
			if (i % newline == newline - 1) {
				x = start->x;
				++y;
			} else
				x += 3;
		}
	} else {
		for (i = 0; i < (int) machine->size; ++i) {
			if (machine->v[i] == 0) {
				wattrset(win, COLOR_PAIR(10));
				mvwprintw(win, y, x, "%02d", machine->v[i]);
			} else {
				wattrset(win, machine->color);
				mvwprintw(win, y, x, "%02d", machine->v[i]);
			}
			if (i % newline == newline - 1) {
				x = start->x;
				++y;
			} else
				x += 3;
		}
	}
	wrefresh(win);
}

static void
shuffle(GARAPON *machine)
{
	size_t i, j, temp;

	for (i = machine->size - 1; i > 0; --i) {
		j = (size_t) ((i + 1) * rnd());
		temp = machine->v[i];
		machine->v[i] = machine->v[j];
		machine->v[j] = temp;
	}
}

static void
print_mid(WINDOW *win, int starty, int startx, int width, const char *string)
{
	float temp;
	int x, y;
	int length;

	if(win == NULL)
		win = stdscr;
	getyx(win, y, x);
	if(startx != 0)
		x = startx;
	if(starty != 0)
		y = starty;
	if(width == 0)
		width = 80;

	length = strlen(string);
	temp = (width - length) / 2;
	x = startx + (int) temp;
	mvwprintw(win, y, x, "%s", string);
	wrefresh(win);
}

static void
print_item_name(WINDOW *win, const char *name)
{
	if (win == NULL)
		win = stdscr;
	werase(win);
	print_mid(win, 0, 0, COLS, name);
	wrefresh(win);
}

#define DSMAX 100
#define DSMIN 0

static void
distsort(int n, const vector a, vector b)
{
	int i, x;
	static int count[DSMAX - DSMIN + 1];

	for (i = 0; i <= DSMAX - DSMIN; ++i)
		count[i] = 0;
	for (i = 0; i < n; ++i)
		++count[a[i] - DSMIN];
	for (i = 1; i <= DSMAX - DSMIN; ++i)
		count[i] += count[i - 1];
	for (i = n - 1; i >= 0; --i) {
		x = a[i] - DSMIN;
		b[--count[x]] = a[i];
	}
}

static void
init_curses(void)
{
	initscr();
	cbreak();
	noecho();
	curs_set(0);
	keypad(stdscr, true);
}

static void
setup_colors(int n)
{
	if (has_colors()) {

		start_color();

		init_pair(10, COLOR_BLACK,   COLOR_BLACK);
		init_pair(11, COLOR_RED,     COLOR_BLACK);
		init_pair(12, COLOR_GREEN,   COLOR_BLACK);
		init_pair(13, COLOR_YELLOW,  COLOR_BLACK);
		init_pair(14, COLOR_BLUE,    COLOR_BLACK);
		init_pair(15, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(16, COLOR_CYAN,    COLOR_BLACK);
		init_pair(17, COLOR_WHITE,   COLOR_BLACK);

		init_pair(20, COLOR_BLACK,   COLOR_WHITE);

		switch (n) {
		case 1:
			init_pair(1, COLOR_RED,     COLOR_BLACK);
			init_pair(2, COLOR_YELLOW,  COLOR_BLACK);
			init_pair(3, COLOR_WHITE,   COLOR_BLACK);
			init_pair(4, COLOR_GREEN,   COLOR_BLACK);
			init_pair(5, COLOR_CYAN,    COLOR_BLACK);
			init_pair(6, COLOR_BLUE,    COLOR_BLACK);
			init_pair(7, COLOR_MAGENTA, COLOR_BLACK);
			break;
		case 2:
			init_pair(1, COLOR_YELLOW,  COLOR_BLACK);
			init_pair(2, COLOR_WHITE,   COLOR_BLACK);
			init_pair(3, COLOR_GREEN,   COLOR_BLACK);
			init_pair(4, COLOR_CYAN,    COLOR_BLACK);
			init_pair(5, COLOR_BLUE,    COLOR_BLACK);
			init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
			init_pair(7, COLOR_RED,     COLOR_BLACK);
			break;
		case 3:
			init_pair(1, COLOR_WHITE,   COLOR_BLACK);
			init_pair(2, COLOR_GREEN,   COLOR_BLACK);
			init_pair(3, COLOR_CYAN,    COLOR_BLACK);
			init_pair(4, COLOR_BLUE,    COLOR_BLACK);
			init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
			init_pair(6, COLOR_RED,     COLOR_BLACK);
			init_pair(7, COLOR_YELLOW,  COLOR_BLACK);
			break;
		case 4:
			init_pair(1, COLOR_GREEN,   COLOR_BLACK);
			init_pair(2, COLOR_CYAN,    COLOR_BLACK);
			init_pair(3, COLOR_BLUE,    COLOR_BLACK);
			init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
			init_pair(5, COLOR_RED,     COLOR_BLACK);
			init_pair(6, COLOR_YELLOW,  COLOR_BLACK);
			init_pair(7, COLOR_WHITE,   COLOR_BLACK);
			break;
		case 5:
			init_pair(1, COLOR_CYAN,    COLOR_BLACK);
			init_pair(2, COLOR_BLUE,    COLOR_BLACK);
			init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
			init_pair(4, COLOR_RED,     COLOR_BLACK);
			init_pair(5, COLOR_YELLOW,  COLOR_BLACK);
			init_pair(6, COLOR_WHITE,   COLOR_BLACK);
			init_pair(7, COLOR_GREEN,   COLOR_BLACK);
			break;
		case 6:
			init_pair(1, COLOR_MAGENTA, COLOR_BLACK);
			init_pair(2, COLOR_BLUE,    COLOR_BLACK);
			init_pair(3, COLOR_CYAN,    COLOR_BLACK);
			init_pair(4, COLOR_GREEN,   COLOR_BLACK);
			init_pair(5, COLOR_WHITE,   COLOR_BLACK);
			init_pair(6, COLOR_YELLOW,  COLOR_BLACK);
			init_pair(7, COLOR_RED,     COLOR_BLACK);
			break;
		case 7:
			init_pair(1, COLOR_BLUE,    COLOR_BLACK);
			init_pair(2, COLOR_CYAN,    COLOR_BLACK);
			init_pair(3, COLOR_GREEN,   COLOR_BLACK);
			init_pair(4, COLOR_WHITE,   COLOR_BLACK);
			init_pair(5, COLOR_YELLOW,  COLOR_BLACK);
			init_pair(6, COLOR_RED,     COLOR_BLACK);
			init_pair(7, COLOR_MAGENTA, COLOR_BLACK);
			break;
		case 8:
			init_pair(1, COLOR_CYAN,    COLOR_BLACK);
			init_pair(2, COLOR_GREEN,   COLOR_BLACK);
			init_pair(3, COLOR_WHITE,   COLOR_BLACK);
			init_pair(4, COLOR_YELLOW,  COLOR_BLACK);
			init_pair(5, COLOR_RED,     COLOR_BLACK);
			init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
			init_pair(7, COLOR_BLUE,    COLOR_BLACK);
			break;
		case 9:
			init_pair(1, COLOR_GREEN,   COLOR_BLACK);
			init_pair(2, COLOR_WHITE,   COLOR_BLACK);
			init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
			init_pair(4, COLOR_RED,     COLOR_BLACK);
			init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
			init_pair(6, COLOR_BLUE,    COLOR_BLACK);
			init_pair(7, COLOR_CYAN,    COLOR_BLACK);
			break;
		case 10:
			init_pair(1, COLOR_WHITE,   COLOR_BLACK);
			init_pair(2, COLOR_YELLOW,  COLOR_BLACK);
			init_pair(3, COLOR_RED,     COLOR_BLACK);
			init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
			init_pair(5, COLOR_BLUE,    COLOR_BLACK);
			init_pair(6, COLOR_CYAN,    COLOR_BLACK);
			init_pair(7, COLOR_GREEN,   COLOR_BLACK);
			break;
		default :
			break;
		}
	}
}

static int
init_game_menu(void)
{
	ITEM **game_items = NULL;
	MENU *game_menu = NULL;
	WINDOW *game_menu_win = NULL;
	int i;
	int ch;
	int selected_item = ERR;
	int n_choices;

	n_choices = (sizeof(choices) / sizeof(choices[0]));
	game_items = (ITEM **) calloc(n_choices + 1, sizeof(ITEM *));
	if (game_items == NULL)
		err(1, NULL);

	for (i = 0; i <  n_choices; ++i) {
		game_items[i] = new_item(choices[i], (char *) NULL);
		set_item_userptr(game_items[i], print_item_name);
	}
	game_items[n_choices] = (ITEM *) NULL;

	game_menu = new_menu((ITEM **) game_items);
	game_menu_win = newwin(10, 20, 2, 0);
	keypad(game_menu_win, true);
	set_menu_win(game_menu, game_menu_win);
	set_menu_sub(game_menu, derwin(game_menu_win, 8, 18, 1 ,0));
	set_menu_mark(game_menu, " * ");
	post_menu(game_menu);
	wrefresh(game_menu_win);

	for (;;) {
		ch = wgetch(game_menu_win);
		switch (ch) {
		case 'j':
		case KEY_DOWN:
			menu_driver(game_menu, REQ_DOWN_ITEM);
			break;
		case 'k':
		case KEY_UP:
			menu_driver(game_menu, REQ_UP_ITEM);
			break;
		case ENTER: {
			ITEM *cur = NULL;
			WINDOW *topbar = NULL;
			void (*p)(WINDOW *, const char *);

			topbar = newwin(1, COLS, 0, 0);
			cur = current_item(game_menu);
			p = item_userptr(cur);
			p(topbar, item_name(cur));
			pos_menu_cursor(game_menu);
			selected_item = item_index(cur);

			unpost_menu(game_menu);
			for (i = 0; i < (int) n_choices; ++i) {
				free_item(game_items[i]);
			}
			free_menu(game_menu);
			wrefresh(game_menu_win);
			wrefresh(topbar);
			delwin(game_menu_win);
			delwin(topbar);
			return selected_item;
		}
		case 'q': {
			unpost_menu(game_menu);
			for (i = 0; i < (int) n_choices; ++i) {
				free_item(game_items[i]);
			}
			free_menu(game_menu);
			wrefresh(game_menu_win);
			delwin(game_menu_win);
			finish(0);
		}
		default:
			break;
		}
		wrefresh(game_menu_win);
	}
	return ERR;
}

static void
nowsleep(WINDOW *win, int starty, int startx, size_t width, int count)
{
	float temp;
	unsigned int bit;
	int i;
	int x, y, spercex;
	size_t flen, slen;
	char first_str[] = "nowsleeping";
	char second_str[] = "zzz...";

	if (win == NULL)
		win = stdscr;
	getyx(win, y, x);
	if (startx != 0)
		x = startx;
	if (starty != 0)
		y = starty;
	if (width == 0)
		width = 80;

	flen = strlen(first_str);
	slen = strlen(second_str);
	temp = (width - (flen + slen + 1)) / 2;
	x = startx + (int) temp;
	werase(win);
	wrefresh(win);
	napms(1000);
	mvwprintw(win, y, x, "%s", first_str);
	wrefresh(win);
	napms(500);
	x += flen;
	spercex = x;
	for (bit = 1, i = 0; i < count; ++i) {
		if (bit)
			mvwaddch(win, y, ++x, (i % 6 < 3) ? 'z' : '.');
		else
			mvwaddch(win, y, ++x, ' ');
		if (i % 6 == 5) {
			x = spercex;
			bit ^= 1;
		}
		wrefresh(win);
		napms(150);
	}
	werase(win);
	wrefresh(win);
}

static long
diffnsec(struct timespec *before, struct timespec *after)
{
	after->tv_nsec -= before->tv_nsec;
	if (after->tv_nsec < 0)
		after->tv_nsec += 1000000000;
	return after->tv_nsec;
}

static void
clear_windows(WINDOW **win, size_t n)
{
	size_t i;

	for (i = 0; i < n; ++i) {
		werase(win[i]);
		wrefresh(win[i]);
	}
}

static void
delete_windows(WINDOW **win, size_t n)
{
	size_t i;

	for (i = 0; i < n; ++i)
		delwin(win[i]);
	free(win);
}

float
num_mid(int width, int n)
{
	return ((width - n * 3 + 1) / 2);
}

static void
us_dream(int selected_item)
{
	WINDOW **imac = NULL;
	GARAPON *lmachine = NULL;
	GARAPON *rmachine = NULL;
	struct point p, startp;
	struct timespec before_ts, after_ts;
	vector  v1 = NULL;
	vector  v2 = NULL;
	static int v3;
	int a = 0;
	int ch;
	int i, j;
	int selected_lot;
	size_t ts;
	size_t windows = 5;

	selected_lot = selected_item;
	switch (selected_lot) {
	case 3:
		lmachine = make_machine(US_SIZE,
		    PMAIN_N,
		    PMAIN_S,
		    0,
		    COLOR_PAIR(14));
		rmachine = make_machine(US_SIZE,
		    POWER_N,
		    POWER_S,
		    0,
		    COLOR_PAIR(11));
		if (lmachine == NULL || rmachine == NULL)
			err(1, NULL);
		break;
	case 4:
		lmachine = make_machine(US_SIZE,
		    MMAIN_N,
		    MMAIN_S,
		    0,
		    COLOR_PAIR(16));
		rmachine = make_machine(US_SIZE,
		    MEGA_N,
		    MEGA_S,
		    0,
		    COLOR_PAIR(13));
		if (lmachine == NULL || rmachine == NULL)
			err(1, NULL);
		break;
	default:
		break;
	}

	if ((imac = (WINDOW **) calloc(windows + 1, sizeof(WINDOW *))) == NULL)
		err(1, NULL);

	imac[BOTTOM] = newwin(1, COLS, LINES - 2, 0);
	imac[LBOX] = newwin(14, 30, 3, COLS / 2 - 31);
	imac[RBOX] = newwin(14, 30, 3, COLS / 2 + 1);
	imac[CTRAY] = newwin(3, 21, 18, (COLS - 21) /2 );
	imac[SBOX] = newwin(9, 21, 3, (COLS - 21) / 2);
	imac[windows] = (WINDOW *) NULL;

	for (i = 1; i < (int) windows - 1; ++i) {
		box(imac[i], 0, 0);
		wnoutrefresh(imac[i]);
	}

	for (i = 0; i < lmachine->number; ++i)
		lmachine->v[i] = i + 1;
	for (i = 0; i < rmachine->number; ++i)
		rmachine->v[i] = i + 1;

	p = startp = makepoint(2, 1);
	printvec(imac[LBOX], &startp, 9, lmachine);
	printvec(imac[RBOX], &startp, 9, rmachine);

	print_mid(imac[BOTTOM], 0, 0, COLS, "Press <Enter> key");
	while ((ch = wgetch(imac[BOTTOM])) != ENTER) {
		if (ch == 'q') {
			clear_windows(imac, windows);
			delete_windows(imac, windows);
			finish(0);
		}
	}

	v1 = new_vector(lmachine->sample);
	v2 = new_vector(lmachine->sample);
	for (i = 0; i < lmachine->sample + rmachine->sample; ++i) {
		print_mid(imac[BOTTOM], 0, 0, COLS, "Press <Enter> key");
		wrefresh(imac[BOTTOM]);
		if (clock_gettime(CLOCK_REALTIME, &before_ts))
			err(1, NULL);

		for (j = DAINOBONNOU; j > 0; --j) {
			shuffle(lmachine);
			shuffle(rmachine);
			printvec(imac[LBOX], &startp, 9, lmachine);
			printvec(imac[RBOX], &startp, 9, rmachine);
			wnoutrefresh(imac[LBOX]);
			wnoutrefresh(imac[RBOX]);
			napms(30);
			nodelay(imac[LBOX], true);
			ch = wgetch(imac[LBOX]);
			if (ch == ENTER)
				break;
			else if (ch == 'q') {
				clear_windows(imac, windows);
				delete_windows(imac, windows);
				finish(0);
			}
		}
		if (i < lmachine->sample) {
			do {
				if (clock_gettime(CLOCK_REALTIME, &after_ts))
					err(1, NULL);
				ts = diffnsec(&before_ts, &after_ts) %
				    lmachine->size;
			} while (lmachine->v[ts] == 0);

			v1[i] = lmachine->v[ts];
			lmachine->v[ts] = 0;
			wattrset(imac[CTRAY], lmachine->color);
			mvwprintw(imac[CTRAY],
			    p.y, p.x + STEP(a++), "%02d", v1[i]);
			wrefresh(imac[CTRAY]);
		} else {
			do {
				if (clock_gettime(CLOCK_REALTIME, &after_ts))
					err(1, NULL);
				ts = diffnsec(&before_ts, &after_ts) %
				    rmachine->size;
			} while (rmachine->v[ts] == 0);

			v3 = rmachine->v[ts];
			rmachine->v[ts] = 0;
			wattrset(imac[CTRAY], rmachine->color);
			mvwprintw(imac[CTRAY], p.y, p.x + STEP(a), "%02d", v3);
			wrefresh(imac[CTRAY]);
		}
		printvec(imac[LBOX], &startp, 9, lmachine);
		printvec(imac[RBOX], &startp, 9, rmachine);
		wrefresh(imac[LBOX]);
		wrefresh(imac[RBOX]);
	}
	nowsleep(imac[BOTTOM], 0, 0, COLS, 30);
	clear_windows(imac, windows);

	distsort(lmachine->sample, v1, v2);
	wattrset(imac[SBOX], COLOR_PAIR(17));
	print_mid(imac[SBOX], 1, 0, 21, "winning numbers");

	wattrset(imac[SBOX], lmachine->color);
	p = makepoint(2, 3);
	for (i = 0, a = 0; i < lmachine->sample; ++i, ++a)
		mvwprintw(imac[SBOX], p.y, p.x + STEP(a), "%02d", v2[i]);

	wattrset(imac[SBOX], rmachine->color);
	mvwprintw(imac[SBOX], p.y, p.x + STEP(a), "%02d", v3);
	wrefresh(imac[SBOX]);

	print_mid(imac[BOTTOM], 0, 0, COLS, "'r' to retry, 'q' to exit");
	while ((ch = wgetch(imac[BOTTOM])) != 'r') {
		if (ch == 'q') {
			clear_windows(imac, windows);
			delete_windows(imac, windows);
			finish(0);
		}
	}
	free_vector(v1);
	free_vector(v2);
	free_machine(lmachine);
	free_machine(rmachine);
	clear_windows(imac, windows);
	delete_windows(imac, windows);
}

static void
eu_dream(int selected_item)
{
	WINDOW **emac = NULL;
	GARAPON *lmachine = NULL;
	GARAPON *rmachine = NULL;
	struct point p, startp;
	struct timespec before_ts, after_ts;
	vector v1 = NULL;
	vector v2 = NULL;
	vector v3 = NULL;
	int a = 0;
	int ch;
	int i, j;
	int selected_lot;
	size_t ts;
	size_t windows = 5;

	selected_lot = selected_item;
	switch (selected_lot) {
	case 5:
		lmachine = make_machine(EU_SIZE,
		    SMAIN_N,
		    SMAIN_S,
		    0,
		    COLOR_PAIR(11));
		rmachine = make_machine(LS_SIZE,
		    STARS_N,
		    STARS_S,
		    0,
		    COLOR_PAIR(13));
		if (lmachine == NULL || rmachine == NULL)
			err(1, NULL);
		break;
	default:
		break;
	}

	if ((emac = (WINDOW **) calloc(windows + 1, sizeof(WINDOW *))) == NULL)
		err(1, NULL);

	emac[BOTTOM] = newwin(1, COLS, LINES - 2, 0);
	emac[LBOX] = newwin(14, 30, 3, COLS / 2 - 31);
	emac[RBOX] = newwin(11, 21, 5, COLS / 2 + 1);
	emac[CTRAY] = newwin(3, 24, 18, (COLS - 24) / 2);
	emac[SBOX] = newwin(9, 24, 3, (COLS - 24) / 2);
	emac[windows] = (WINDOW *) NULL;

	for (i = 1; i < (int) windows - 1; ++i) {
		box(emac[i], 0, 0);
		wnoutrefresh(emac[i]);
	}

	for (i = 0; i < lmachine->number; ++i)
		lmachine->v[i] = i + 1;
	for (i = 0; i < rmachine->number; ++i)
		rmachine->v[i] = i + 1;

	p = startp = makepoint(2, 1);
	printvec(emac[LBOX], &startp, 9, lmachine);
	printvec(emac[RBOX], &startp, 6, rmachine);

	print_mid(emac[BOTTOM], 0, 0, COLS, "Press <Enter> key");
	while ((ch = wgetch(emac[BOTTOM])) != ENTER) {
		if (ch == 'q') {
			clear_windows(emac, windows);
			delete_windows(emac, windows);
			finish(0);
		}
	}

	v1 = new_vector(lmachine->sample);
	v2 = new_vector(lmachine->sample);
	v3 = new_vector(rmachine->sample);
	for (i = 0; i < lmachine->sample + rmachine->sample; ++i) {
		print_mid(emac[BOTTOM], 0, 0, COLS, "Press <Enter> key");
		wrefresh(emac[BOTTOM]);
		if (clock_gettime(CLOCK_REALTIME, &before_ts))
			err(1, NULL);

		for (j = DAINOBONNOU; j > 0; --j) {
			if (i < lmachine->sample) {
				shuffle(lmachine);
				printvec(emac[LBOX], &startp, 9, lmachine);
				wnoutrefresh(emac[LBOX]);
			} else {
				shuffle(rmachine);
				printvec(emac[RBOX], &startp, 6, rmachine);
				wnoutrefresh(emac[RBOX]);
			}
			napms(30);
			nodelay(emac[LBOX], true);
			ch = wgetch(emac[LBOX]);
			if (ch == ENTER)
				break;
			else if (ch == 'q') {
				clear_windows(emac, windows);
				delete_windows(emac, windows);
				finish(0);
			}
		}
		if (i < lmachine->sample) {
			do {
				if (clock_gettime(CLOCK_REALTIME, &after_ts))
					err(1, NULL);
				ts = diffnsec(&before_ts, &after_ts) %
				    lmachine->size;
			} while (lmachine->v[ts] == 0);

			v1[i] = lmachine->v[ts];
			lmachine->v[ts] = 0;
			wattrset(emac[CTRAY], lmachine->color);
			mvwprintw(emac[CTRAY],
			    p.y, p.x + STEP(a++), "%02d", v1[i]);
			wrefresh(emac[CTRAY]);
		} else {
			do {
				if (clock_gettime(CLOCK_REALTIME, &after_ts))
					err(1, NULL);
				ts = diffnsec(&before_ts, &after_ts) %
				    rmachine->size;
			} while (rmachine->v[ts] == 0);

			v3[i - lmachine->sample] = rmachine->v[ts];
			rmachine->v[ts] = 0;
			wattrset(emac[CTRAY], rmachine->color);
			mvwprintw(emac[CTRAY], p.y, p.x + STEP(a++),
			    "%02d", v3[i - lmachine->sample]);
			wrefresh(emac[CTRAY]);
		}
		printvec(emac[LBOX], &startp, 9, lmachine);
		printvec(emac[RBOX], &startp, 6, rmachine);
		wrefresh(emac[LBOX]);
		wrefresh(emac[RBOX]);
	}
	nowsleep(emac[BOTTOM], 0, 0, COLS, 30);
	clear_windows(emac, windows);

	distsort(lmachine->sample, v1, v2);
	wattrset(emac[SBOX], COLOR_PAIR(17));
	print_mid(emac[SBOX], 1, 0, 24, "winning numbers");

	wattrset(emac[SBOX], lmachine->color);
	p = makepoint(2, 3);
	for (i = 0, a = 0; i < lmachine->sample; ++i, ++a)
		mvwprintw(emac[SBOX], p.y, p.x + STEP(a), "%02d", v2[i]);

	wattrset(emac[SBOX], rmachine->color);
	mvwprintw(emac[SBOX], p.y, p.x + STEP(a++), "%02d", MIN(v3[0], v3[1]));
	mvwprintw(emac[SBOX], p.y, p.x + STEP(a), "%02d", MAX(v3[0], v3[1]));
	wrefresh(emac[SBOX]);

	print_mid(emac[BOTTOM], 0, 0, COLS, "'r' to retry, 'q' to exit");
	while ((ch = wgetch(emac[BOTTOM])) != 'r') {
		if (ch == 'q') {
			clear_windows(emac, windows);
			delete_windows(emac, windows);
			finish(0);
		}
	}
	free_vector(v1);
	free_vector(v2);
	free_vector(v3);
	free_machine(lmachine);
	free_machine(rmachine);
	clear_windows(emac, windows);
	delete_windows(emac, windows);
}

static void
ja_dream(int selected_item)
{
	WINDOW **imac = NULL;
	GARAPON *mmachine = NULL;
	struct point p, startp;
	struct timespec before_ts, after_ts;
	vector v1 = NULL;
	vector v2 = NULL;
	vector v3 = NULL;
	int selected_lot;
	int a = 0;
	int ch;
	int i, j;
	size_t ts;
	size_t windows = 4;

	selected_lot = selected_item;
	switch (selected_lot) {
	case 0:
		mmachine = make_machine(JA_SIZE,
		    MIN_L_N,
		    MIN_L_S,
		    MIN_L_O,
		    NOT_SET);
		if (mmachine == NULL)
			err(1, NULL);
		break;
	case 1:
		mmachine = make_machine(JA_SIZE,
		    L_SIX_N,
		    L_SIX_S,
		    L_SIX_O,
		    NOT_SET);
		if (mmachine == NULL)
			err(1, NULL);
		break;
	case 2:
		mmachine = make_machine(JA_SIZE,
		    L_SEV_N,
		    L_SEV_S,
		    L_SEV_O,
		    NOT_SET);
		if (mmachine == NULL)
			err(1, NULL);
		break;
	default:
		break;
	}

	if ((imac = (WINDOW **) calloc(windows + 1, sizeof(WINDOW *))) == NULL)
		err(1, NULL);

	imac[BOTTOM] = newwin(1, COLS, LINES - 2, 0);
	imac[MBOX] = newwin(12, 24, 3, (COLS - 24) >> 1);
	imac[MTRAY] = newwin(3, 24, 15, (COLS - 24) >> 1);
	imac[OTRAY] = newwin(3, 24, 18, (COLS - 24) >> 1);
	imac[windows] = (WINDOW *) NULL;

	for (i = 1; i < (int) windows; ++i) {
		box(imac[i], 0, 0);
		wnoutrefresh(imac[i]);
	}

	for (i = 0; i < mmachine->number; ++i)
		mmachine->v[i] = i + 1;

	p = startp = makepoint(2, 1);
	printvec(imac[MBOX], &startp, 7, mmachine);
	wnoutrefresh(imac[MBOX]);

	print_mid(imac[BOTTOM], 0, 0, COLS, "Press <Enter> key");
	while ((ch = wgetch(imac[BOTTOM])) != ENTER) {
		if (ch == 'q') {
			clear_windows(imac, windows);
			delete_windows(imac, windows);
			finish(0);
		}
	}
	werase(imac[BOTTOM]);
	wrefresh(imac[BOTTOM]);

	v1 = new_vector(mmachine->sample);
	v2 = new_vector(mmachine->sample);
	v3 = new_vector(mmachine->omake);
	for (i = 0; i < mmachine->sample + mmachine->omake; ++i) {
		print_mid(imac[BOTTOM], 0, 0, COLS, "Press <Enter> key");
		wrefresh(imac[BOTTOM]);
		if (clock_gettime(CLOCK_REALTIME, &before_ts))
			err(1, NULL);

		for (j = DAINOBONNOU; j > 0; --j) {
			shuffle(mmachine);
			printvec(imac[MBOX], &startp, 7, mmachine);
			wrefresh(imac[MBOX]);
			napms(30);
			nodelay(imac[BOTTOM], true);
			ch = wgetch(imac[BOTTOM]);
			if (ch == ENTER)
				break;
			else if (ch  == 'q') {
				clear_windows(imac, windows);
				delete_windows(imac, windows);
				finish(0);
			}
		}
		werase(imac[BOTTOM]);
		wrefresh(imac[BOTTOM]);

		do {
			if (clock_gettime(CLOCK_REALTIME, &after_ts))
				err(1, NULL);
			ts = diffnsec(&before_ts, &after_ts) %
			    mmachine->size;
		} while (mmachine->v[ts] == 0);

		if (i < mmachine->sample) {
			v1[i] = mmachine->v[ts];
			mmachine->v[ts] = 0;
			mvwprintw(imac[MTRAY], p.y, p.x + STEP(a++), "%02d",
			    colorful(imac[MTRAY], v1[i]));
			wrefresh(imac[MTRAY]);
		} else {
			v3[i - mmachine->sample] = mmachine->v[ts];
			mmachine->v[ts] = 0;
			mvwprintw(imac[OTRAY], p.y, p.x, "%02d",
			    colorful(imac[OTRAY], v3[i - mmachine->sample]));
			wrefresh(imac[OTRAY]);
			p.x += 3;
		}
		printvec(imac[MBOX], &startp, 7, mmachine);
		wrefresh(imac[MBOX]);
	}
	nowsleep(imac[BOTTOM], 0, 0, COLS, 30);
	clear_windows(imac, windows);

	distsort(mmachine->sample, v1, v2);
	wattron(imac[MBOX], COLOR_PAIR(17));
	print_mid(imac[MBOX], 1, 0, 24, "winning numbers");
	print_mid(imac[MBOX], 5, 0, 24, "omake");
	wattroff(imac[MBOX], COLOR_PAIR(17));

	p = makepoint((int) num_mid(24, mmachine->sample), 3);
	for (i = 0, a = 0; i < mmachine->sample; ++i, ++a)
		mvwprintw(imac[MBOX], p.y, p.x + STEP(a),
		    "%02d", colorful(imac[MBOX], v2[i]));

	p = makepoint((int) num_mid(24, mmachine->omake), 7);
	if (mmachine->omake == 1)
		mvwprintw(imac[MBOX], p.y, p.x,
		    "%02d", colorful(imac[MBOX], v3[0]));
	else if (mmachine->omake == 2) {
		mvwprintw(imac[MBOX], p.y, p.x,
		    "%02d", colorful(imac[MBOX], MIN(v3[0], v3[1])));
		mvwprintw(imac[MBOX], p.y, p.x + 3,
		    "%02d", colorful(imac[MBOX], MAX(v3[0], v3[1])));
	}
	wrefresh(imac[MBOX]);

	print_mid(imac[BOTTOM], 0, 0, COLS, "'r' to retry, 'q' to exit");
	while ((ch = wgetch(imac[BOTTOM])) != 'r') {
		if (ch == 'q') {
			clear_windows(imac, windows);
			delete_windows(imac, windows);
			finish(0);
		}
	}
	free_vector(v1);
	free_vector(v2);
	free_vector(v3);
	free_machine(mmachine);
	clear_windows(imac, windows);
	delete_windows(imac, windows);
}

int
main(int argc, char *argv[])
{
	WINDOW *titlebar = NULL;
	WINDOW *messagebar = NULL;
	int selected_item;
	int ch;
	bool selected = false;

	signal(SIGINT, finish);
#ifdef HAVE_SRANDOMDEV
	srandomdev();
#else
	srandom((unsigned) time(NULL));
#endif
	init_curses();
	setup_colors(random() % 10 + 1);

	titlebar = newwin(1, COLS, 0, 0);
	messagebar = newwin(1, COLS, LINES - 2, 0);

	do {
		werase(titlebar);
		print_mid(titlebar, 0, 0, COLS, "garapon");
		wrefresh(titlebar);
		werase(messagebar);
		wrefresh(messagebar);

		if ((selected_item = init_game_menu()) == ERR)
			finish(1);

		switch (selected_item) {
		case 0:
			ja_dream(selected_item);
			selected = true;
			break;
		case 1:
			ja_dream(selected_item);
			selected = true;
			break;
		case 2:
			ja_dream(selected_item);
			selected = true;
			break;
		case 3:
			us_dream(selected_item);
			selected = true;
			break;
		case 4:
			us_dream(selected_item);
			selected = true;
			break;
		case 5:
			eu_dream(selected_item);
			selected = true;
			break;
		case 6:
			mvwprintw(messagebar, 0, 0, "'q' to exit");
			selected = false;
			break;
		case 7:
			goto endgame;
		default:
			break;
		}
		nodelay(titlebar, selected);
		wrefresh(messagebar);
	} while ((ch = wgetch(titlebar)) != 'q');

endgame:
	werase(titlebar);
	werase(messagebar);
	wrefresh(titlebar);
	wrefresh(messagebar);
	delwin(titlebar);
	delwin(messagebar);
	finish(0);
}

void
finish(int status)
{
	endwin();
	exit(status);
}
