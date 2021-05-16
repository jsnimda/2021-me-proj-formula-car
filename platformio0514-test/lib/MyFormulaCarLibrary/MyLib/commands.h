#ifndef commands_h
#define commands_h

#include "LineCommand.h"

// ============
// command set
// ============

inline void core_r(Line& c) {
  if (c.on("syn")) {
    int ind = c.ind;
    if (c.on("int") && c.onInt()) {
      c.setResponse(stringf("ACK int: %ld", c.rint));
    }
    c.sub(ind);
    if (c.on("double") && c.onDouble()) {
      c.setResponse(stringf("ACK double: %f", c.rdouble));
    }
    c.sub(ind);
  }
}


#endif // commands_h
