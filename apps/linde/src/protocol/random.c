//random.c

#include "random.h"
#include "zephyr.h"

#define RANDOM_MAX 0x7FFFFFFF

static long do_random(unsigned long *value)
{
    long quotient, remainder, t;

    quotient = *value / 127773L;
    remainder = *value % 127773L;
    t = 16807L * remainder - 2836L * quotient;
    if (t <= 0)
        t += 0x7FFFFFFFL;
    return ((*value = t) % ((unsigned long)RANDOM_MAX + 1));
}

static unsigned long next = 1;

int rand(void)
{
    return do_random(&next);
}

void srand(unsigned int seed)
{
    next = k_cycle_get_32();
}

unsigned long time(long *val)
{
    return k_cycle_get_32();
}
