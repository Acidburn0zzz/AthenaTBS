#pragma once

struct Athena_Button{
    int x, y;
    unsigned w, h;
    const char *text;

    void *arg;
    void (*callback)(void *);
    unsigned clicked;
};


struct Athena_ButtonList{
    struct Athena_Button button;
    struct Athena_ButtonList *next;
};
