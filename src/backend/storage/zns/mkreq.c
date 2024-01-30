#include "storage/mkreq.h"
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

#define ZBD_REPORT_MAX_NR_ZONE 8192
#define OFFSET_OF_ZONE 2147483648

static int dev_fd;
static ZslbaInfo *zslbaInfo;
static struct zbd_info *zbdi;
static struct zbd_zone *zones;

int
InitZslbaArray()
{
	unsigned int	nz = 904L;
	int	ret;

	zbd_set_log_level(ZBD_LOG_ERROR);
	zbdi = NULL;
	dev_fd = zbd_open("/dev/nvme0n2", O_RDWR, NULL);

	if (zslbaInfo == NULL)
		zslbaInfo = (struct ZslbaInfo *) calloc(MAX_OPEN_ZONE, sizeof(ZslbaInfo));

	if (zbdi == NULL)
		zbdi = (struct zbd_info *) calloc(nz, sizeof(struct zbd_info));

	ret = zbd_get_info(dev_fd, zbdi);

	if (ret == -1) {
		printf("Get device information error!!\n");
	}

	return ret;
}

int
ParseRelation(SMgrRelation reln)
{
	char		*name;
	char		*ptr;
	int	number;
	Relation target;

	target = RelationIdGetRelation(reln->smgr_rnode.node.relNode);
	name = RelationGetRelationName(target);
	RelationClose(target);

	ptr = strstr(name, "hyper");
	
	if (ptr == NULL || ptr + 6 == NULL)
	{
		return 0;
	}

	number = (int) (*(ptr + 6) - '0');

	return number;
}

int
GetZoneNumber(int index)
{
	int	ret;

	if (zslbaInfo[index] == 0)
	{
		zslbaInfo[index] = AllocateZslba() + 1;
	}

	ret = CheckZoneState(zslbaInfo[index]);

	if (ret == -1) return -1;

	if (ret != ZBD_ZONE_COND_IMP_OPEN &&
			ret != ZBD_ZONE_COND_EXP_OPEN)
		zslbaInfo[index] = AllocateZslba() + 1;

	return zslbaInfo[index];
}

unsigned long long
GetZslba(int number)
{
	unsigned int	nz = 904L;
	int	ret = 0;

	zones = (struct zbd_zone *) calloc(nz, sizeof(struct zbd_zone));
	ret = zbd_report_zones(dev_fd, 0, OFFSET_OF_ZONE * nz, ZBD_RO_ALL, zones, &nz);

	if (ret == -1)
	{
		printf("Report Error!!\n");
		free(zones);
		return -1;
	}

	return zones[--number].start;
}

int
CheckZoneState(int number)
{
	unsigned int	nz = 904L;
	int	ret;

	zones = (struct zbd_zone *) calloc(nz, sizeof(struct zbd_zone));
	ret = zbd_report_zones(dev_fd, 0, OFFSET_OF_ZONE * nz, ZBD_RO_ALL, zones, &nz);

	if (ret == -1)
	{
		printf("Report Error!!\n");
		free(zones);
		return ret;
	}

	ret = zones[--number].cond;
	free(zones);
	return ret;
}

int
AllocateZslba()
{
	unsigned int nz = 904;

	int	ret;
	int	i;
	int	new_zslba;

	zones = (struct zbd_zone*) calloc(nz, sizeof(struct zbd_zone));

	ret = zbd_report_zones(dev_fd, 0, OFFSET_OF_ZONE * nz, ZBD_RO_ALL, zones, &nz);

	if (ret == -1)
	{
		printf("Report Error!!\n");
		free(zones);
		return ret;
	}

	for (i = 0; i < 904; i++)
	{
		if (zones[i].cond != ZBD_ZONE_COND_EMPTY) continue;

		ret = zbd_open_zones(dev_fd, zones[i].start, zones[i].len);

		if (ret == -1) {
			printf("Zone open error!!\n");
			free(zones);
			return ret;
		}

		return i;
	}

	printf("No more empty zone\n");
	return -1;
}

int
ZoneAppend(unsigned long long zslba, char *buf, int size)
{
	ZnsAppendReq args;
	int 	ret;
	long long unsigned int	result;
	int	nblocks, err = 0;
	int	dfd = STDIN_FILENO;

	ret = nvme_get_nsid(dev_fd, &args.nsid);

	if (ret < 0)
	{
		printf("Get namespace error!!\n");
	}

	nblocks = (size / 4096) - 1;

	args.args_size = sizeof(args);
	args.zslba = zslba;
	args.timeout = NVME_DEFAULT_IOCTL_TIMEOUT;
	args.result = &result;
	args.data_len = 4096;
	args.data = buf;
	args.nlb = 0;
	args.fd = dev_fd;

	ret = nvme_zns_append(&args);

	printf("%d\n", ret);

/*	if (!ret)
		printf("Success appended data to LBA %:"PRIx64"\n", (uint64_t)result);
	else
		printf("Zone append error!! : %d\n", errno); */

	return ret;
}
