#ifndef SR_RECORD_H
#define SR_RECORD_H
#include "encoder/encoder.h"

void srRecordInit(void);
void srRecordExit(void);
const SREncoder* srRecordVideo(void);

#endif
