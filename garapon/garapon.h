/* garapon.h */

#ifndef GARAPON_H
#define GARAPON_H

#ifdef BONNOU
# include "bonnou.h"
#else
# define BONNOU 0
# define DAINOBONNOU 108
#endif

#define US_SIZE 108
#define PMAIN_N (66 + BONNOU)
#define PMAIN_S 5
#define POWER_N (23 + BONNOU)
#define POWER_S 1
#define MMAIN_N (67 + BONNOU)
#define MMAIN_S 5
#define MEGA_N  (22 + BONNOU)
#define MEGA_S  1
#define EU_SIZE 108
#define LS_SIZE 54
#define SMAIN_N (47 + BONNOU)
#define SMAIN_S 5
#define STARS_N (9 + BONNOU)
#define STARS_S 2
#define JA_SIZE 70
#define MIN_L_N (28 + BONNOU)
#define MIN_L_S 5
#define MIN_L_O 1
#define L_SIX_N (40 + BONNOU)
#define L_SIX_S 6
#define L_SIX_O 1
#define L_SEV_N (34 + BONNOU)
#define L_SEV_S 7
#define L_SEV_O 2

#undef STEP
#define STEP(a) (a * 3)
#undef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))

enum
{
	BOTTOM,
	LBOX,
	RBOX,
	CTRAY,
	SBOX
};

enum
{
	MBOX = 1,
	MTRAY,
	OTRAY
};

typedef int *vector;

struct point {
	int x;
	int y;
};

typedef struct {
	vector v;
	chtype color;
	int number;
	int sample;
	int omake;
	size_t size;
} GARAPON;

#endif /* GARAPON_H */
