#include <stdlib.h>
#include <string.h>
#include <librtmp/log.h>
#include "example.h"

static int PrintCounts(struct RTMP *r, const RTMPPacket *packet, void *ctx);

static void *NewInstance(RTMP *r)
{
  ExampleData *data = malloc(sizeof(ExampleData));
  if(!data)
    return NULL;

  memset(data, 0, sizeof(ExampleData));

  data->CBHandle =
    RTMP_AttachCallback(r, RTMP_CALLBACK_PACKET,
                        (void(*)(void))PrintCounts, data);

  return data;
}

static void FreeInstance(RTMP *r, void *ctx)
{
  ExampleData *data = ctx;
  if (!data)
    return;

  RTMP_DetachCallback(r, data->CBHandle);
  free(data);
}

static void ParseOptions(const AVal *opt, const AVal *arg, void *ctx)
{
  ExampleData *data = ctx;
  if(!data)
    return;

  if (opt->av_len != strlen("counts") || 
      strcasecmp(opt->av_val, "counts") != 0)
    return;

  data->enabled = strtol(arg->av_val, NULL, 0);
}

static int PrintCounts(struct RTMP *r, const RTMPPacket *packet, void *ctx)
{
  ExampleData *data = ctx;
  if (!data)
    return RTMP_CB_ERROR_STOP;

  if (!data->enabled)
    return RTMP_CB_NOT_HANDLED;

  data->packet_count += 1;
  data->total_bytes += packet->m_nBodySize;
  
  RTMP_Log(RTMP_LOGINFO, "Number of received packets: %d", data->packet_count);
  RTMP_Log(RTMP_LOGINFO, "Total RTMP payload bytes: %d", data->total_bytes);

  return RTMP_CB_NOT_HANDLED; /* Continue with the normal processing */
}
