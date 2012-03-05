#ifndef __EXAMPLE_PLUGIN_H__
#define __EXAMPLE_PLUGIN_H__
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *      Copyright (C) 2008-2009 Andrej Stepanchuk
 *      Copyright (C) 2009-2010 Howard Chu
 *      Copyright (C) 2012 Antti Ajanki
 *
 *  This file is part of librtmp.
 *
 *  librtmp is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1,
 *  or (at your option) any later version.
 *
 *  librtmp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with librtmp see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/lgpl.html
 */

/*
 * A simple example librtmp plugin to illustrate the plugin API. The
 * plugin prints the number of received RMTP packets and payload
 * bytes.
 *
 * Once installed to $(LIBDIR)/librtmp/plugins or to
 * ~/.librtmp/plugins, the plugin can be run on any RMTP stream by
 * adding "counts=1" to the URL options.
 */

#include <librtmp/plugin.h>
#include <librtmp/rtmp.h>

typedef struct ExampleData {
  long enabled;
  int packet_count;
  uint32_t total_bytes;
  RTMPCallbackHandle CBHandle;
} ExampleData;

static void *NewInstance(RTMP *r);
static void FreeInstance(RTMP *r, void *data);
static void ParseOptions(const AVal *opt, const AVal *arg, void *ctx);

static RTMPPluginOption exampleoptions[] =
  { {AVC("counts"), AVC("int"), AVC("If non-zero, print packet sizes and count"), ParseOptions},
    {{0}, {0}, {0}, 0}};

RTMP_Plugin plugin = 
{
  RTMP_PLUGIN_API,
  AVC("example"),
  AVC("1.0"),
  AVC("Antti Ajanki <antti.ajanki@iki.fi>"),
  AVC("http://rtmpdump.mplayerhq.hu/"),
  exampleoptions,
  NewInstance,
  FreeInstance
};

RTMP_PLUGIN_REGISTER(plugin); /* must be called to register the plugin */

#endif
