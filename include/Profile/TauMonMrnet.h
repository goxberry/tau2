#ifndef _TAU_MON_MRNET_H
#define _TAU_MON_MRNET_H

#include <stdint.h>

#include "mrnet/Types.h"

// Convenience macros
#define STREAM_FLUSHSEND(stream, tag, format, ...)         \
  if ((stream->send(tag, format, __VA_ARGS__) == -1) ||	   \
      (stream->flush() == -1)) {			   \
    printf("Stream send failure [format: %s]\n", format);  \
    exit(-1);						   \
  }

// Protocol definitions to be shared between any C++ ToM front-ends and the
//   application back-ends.

const int NUM_TOM_BASE_TAGS = 4;

enum {
  TOM_CONTROL = FirstApplicationTag,
  TOM_EXIT,         // shutdown application

  PROT_DATA_READY,  // data ready signal from application
  PROT_EXIT,        // exit protocol loop

  PROT_UNIFY,       // event unification protocol
  PROT_BASESTATS,   // basic statistics protocol
  PROT_HIST,        // histogramming protocol

  PROT_CLASSIFIER,  // classification protocol
  PROT_CLUST_KMEANS // k-means clustering protocol
};

#endif /* _TAU_MON_MRNET_H */
