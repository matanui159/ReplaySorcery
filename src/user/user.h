#ifndef SR_USER_H
#define SR_USER_H

typedef struct SRUser {
   void (*wait)(struct SRUser* user);
   void (*destroy)(struct SRUser* user);
   void* extra;
} SRUser;

void srUserDestroy(SRUser* user);
void srUserWait(SRUser* user);

#endif
