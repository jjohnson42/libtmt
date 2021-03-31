#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>
#include "tmt.h"
#define HASHSIZE 2053
#define MAXNAMELEN 256
#define MAXDATALEN 8192
struct terment {
    struct terment *next;
    char *name;
    TMT *vt;
};

#define SETNODE 1
#define WRITE 2
#define READBUFF 0
static struct terment *buffers[HASHSIZE];

unsigned long hash(char *str)
/* djb2a */
{
    unsigned long idx = 5381;
    int c;

    while (c = *str++)
        idx = ((idx << 5) + idx) + c;
    return idx % HASHSIZE;
}

TMT *get_termentbyname(char *name) {
    struct terment *ret;
    for (ret = buffers[hash(name)]; ret != NULL; ret = ret->next)
        if (strcmp(name, ret->name) == 0)
            return ret->vt;
    return NULL;
}

TMT *set_termentbyname(char *name) {
    struct terment *ret;
    int idx;

    idx = hash(name);
    for (ret = buffers[idx]; ret != NULL; ret = ret->next)
        if (strcmp(name, ret->name) == 0)
            return ret->vt;
    ret = (struct terment *)malloc(sizeof(*ret));
    ret->next = buffers[idx];
    ret->name = strdup(name);
    ret->vt = tmt_open(31, 100, NULL, NULL, L"→←↑↓■◆▒°±▒┘┐┌└┼⎺───⎽├┤┴┬│≤≥π≠£•");
    buffers[idx] = ret;
    return ret->vt;
}

void dump_vt(TMT* outvt) {
    const TMTSCREEN *out = tmt_screen(outvt);
    const TMTPOINT *curs = tmt_cursor(outvt);
    int line, idx, deferredlines, deferredspaces;
    bool printedline;
    deferredlines = 0;
    for (line = 0; line < out->nline; line++) {
        deferredspaces = 0;
        printedline = false;
        for (idx = 0; idx < out->ncol; idx++) {
            if (out->lines[line]->chars[idx].c == L' ') {
                deferredspaces += 1;
            } else {
                while (deferredlines) {
                    wprintf(L"\r\n");
                    deferredlines -= 1;
                }
                while (deferredspaces > 0) {
                    wprintf(L" ");
                    deferredspaces -= 1;
                }
                printedline = true;
                wprintf(L"%lc", out->lines[line]->chars[idx].c);
            }
        }
        if (printedline)
            wprintf(L"\r\n");
        else
            deferredlines  += 1;
    }
    fflush(stdout);
    wprintf(L"\x1b[%ld;%ldH", curs->r, curs->c);
    fflush(stdout);
    write(1, "\x00", 1);
}

int main(int argc, char* argv[]) {
    int cmd, length;
    setlocale(LC_ALL, "");
    char cmdbuf[MAXDATALEN];
    char currnode[MAXNAMELEN];
    TMT *currvt = NULL;
    TMT *outvt = NULL;
    freopen(NULL, "rb", stdin);
    while (1) {
        fread(&cmd, 4, 1, stdin);
        length = cmd & 536870911;
        cmd = cmd >> 29;
        if (cmd == SETNODE) {
            currnode[length] = 0;
            fread(currnode, 1, length, stdin);
            currvt = set_termentbyname(currnode);
        } else if (cmd == WRITE) {
            if (currvt == NULL)
                currvt = set_termentbyname("");
            cmdbuf[length] = 0;
            fread(cmdbuf, 1, length, stdin);
            tmt_write(currvt, cmdbuf, length);
        } else if (cmd == READBUFF) {
            cmdbuf[length] = 0;
            fread(cmdbuf, 1, length, stdin);
            outvt = get_termentbyname(cmdbuf);
            if (outvt != NULL) {
                dump_vt(outvt);
            }
        }
    }
}
