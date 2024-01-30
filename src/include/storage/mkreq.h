#include "postgres.h"

#include <libzbd/zbd.h>
#include <libnvme.h>

#include "utils/rel.h"

#define MAX_OPEN_ZONE 14

typedef int ZslbaInfo;
typedef struct nvme_zns_append_args ZnsAppendReq;

extern int InitZslbaArray(void);

extern int ParseRelation(SMgrRelation reln);

extern int GetZoneNumber(int index);

extern int CheckZoneState(int intdex);

extern unsigned long long GetZslba(int number);

extern int AllocateZslba(void);

extern int ZoneAppend(unsigned long long zslba, char *buffer, int size);
