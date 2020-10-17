/*
 * Copyright (C) 2020  Joshua Minter
 *
 * This file is part of ReplaySorcery.
 *
 * ReplaySorcery is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReplaySorcery is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ReplaySorcery.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef RS_CONTROL_H
#define RS_CONTROL_H
#include <libavutil/avutil.h>

typedef struct RSControl {
   void *extra;
   void (*destroy)(struct RSControl *control);
   int (*wantsSave)(struct RSControl *control);
} RSControl;

#define RS_CONTROL_INIT                                                                  \
   { NULL }

static av_always_inline void rsControlDestroy(RSControl *control) {
   if (control->destroy != NULL) {
      control->destroy(control);
   }
}

static av_always_inline int rsControlWantsSave(RSControl *control) {
   if (control->wantsSave == NULL) {
      return AVERROR(ENOSYS);
   } else {
      return control->wantsSave(control);
   }
}

int rsDebugControlCreate(RSControl *control);
int rsDefaultControlCreate(RSControl *control);

#endif
