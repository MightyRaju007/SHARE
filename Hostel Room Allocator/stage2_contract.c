#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "commonHeaderFile.h"

/* True when two choices name the same hostel and the same floor. Used all
   over the pairing code to compare what two students asked for. */
int sameFloor(const Choice *a, const Choice *b)
{
    return a->floor == b->floor && strcmp(a->hostel, b->hostel) == 0;
}

/* The registration status as text, for messages and for writing the file
   back out. "?" should never appear, it only covers a value outside the enum. */
const char *statusName(Status s)
{
    switch (s) {
        case STATUS_RANDOM: return "RANDOM";
        case STATUS_PENDING: return "PENDING";
        case STATUS_LOCKED: return "LOCKED";
    }
    return "?";
}

/* Reads a choice typed as the hostel code and floor number, like AB1, into
   a Choice record. Returns 0 if the text is not in that shape. */
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

/* The reverse of parseChoice, turns a Choice back into text like AB1. The
   buffer must have room for the two letters, the digits and the terminator. */
void formatChoice(const Choice *c, char *buf)
{
    sprintf(buf, "%s%d", c->hostel, c->floor);
}
