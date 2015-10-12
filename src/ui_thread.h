#pragma once
#include "game.h"

/* In reality this must NOT be run on a different thread, since basically all
 * UI setups can't handle that, or at least require much more complex setup for
 * multi-threaded access. Instead, this is simply the function to call for each
 * frame of the UI. We use this configuration to ease use for delegates or handler
 * code on certain UI setups (Android and Cocoa being good examples).
 *
 * We use the same API as the server thread for consistency, and so that we could,
 * if we wanted, run this on a different thread. But don't do that unless you really
 * know what you are doing!
 * 
 * 
 */
void Athena_UIThreadWrapper(void *that);
int Athena_UIThread(struct Athena_GameState *that);

/* This should be called once per frame. Athena_UIThread[Wrapper] does this, sort of. */
int Athena_UIThreadFrame(struct Athena_GameState *that);

void Athena_UIInit(struct Athena_UI *ui);

/* Returns the X/Y on the field of a click point indicated as x, y */
int Athena_UIClickAt(const struct Athena_UI *ui, int * /* in/out */ x, int * /* in/out */ y);
int Athena_UIClickAt2(const struct Athena_UI *ui, int x_in, int y_in, int *x_out, int *y_out);
