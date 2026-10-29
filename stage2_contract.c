#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "commonHeaderFile.h"

int sameFloor(const Choice *a, const Choice *b)
{
    return a->floor == b->floor && strcmp(a->hostel, b->hostel) == 0;
}

const char *statusName(Status s)
{
    switch (s) {
        case STATUS_RANDOM: return "RANDOM";
        case STATUS_PENDING: return "PENDING";
        case STATUS_LOCKED: return "LOCKED";
    }
    return "?";
}

/* two capital letters followed by the floor number, e.g. AB1 */
int parseChoice(const char *text, Choice *c)
{
    if (!isupper((unsigned char)text[0]) || !isupper((unsigned char)text[1]))
        return 0;
    if (!isdigit((unsigned char)text[2]))
        return 0;
    c->hostel[0] = text[0];
    c->hostel[1] = text[1];
    c->hostel[2] = '\0';
    c->floor = atoi(text + 2);
    return 1;
}

void formatChoice(const Choice *c, char *buf)
{
    sprintf(buf, "%s%d", c->hostel, c->floor);
}
