#ifndef __PLUGIN_H__
#define __PLUGIN_H__
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

#include <stddef.h>
#include <stdint.h>
#include "rtmp.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct RTMPPluginOption {
  const AVal name;  /* Name of the option in SetupURL string */
  const AVal type;  /* Type name to be shown in the help screen */
  const AVal usage; /* A short description in the help screen */
  void (*parseOption)(const AVal *name, const AVal *val, void *ctx);
    /* Function that will be called if the SetupURL string contains
     * this option. ctx is the user data pointer returned by the
     * create hook. */
} RTMPPluginOption;

typedef struct RTMP_Plugin
{
  uint32_t requiredAPIVersion; /* Expected API version (soname). If
                                * the librtmp, which is trying to load
                                * this plugin, has different API
                                * version the plugin will not be
                                * loaded. */
  const AVal name;             /* Name of the plugin */
  const AVal version;          /* Plugin version */
  const AVal author;           /* Plugin author name and email */
  const AVal homepage;         /* Address where the latest version can
                                * be downloaded */
  RTMPPluginOption *options;   /* Array of RTMPPluginOptions. A plugin
                                * instance will be created if an
                                * option in the SetupURL string
                                * matched to one of these. The last
                                * item must be { 0 }. */
  void *(*create)(RTMP *r); 
    /* This is the first hook to be called if one of plugin's
     * registered URL option is detected. This will be called only
     * once even if there are multiple matching URL options. The
     * return value is a pointer to plugin's private data. It will be
     * passed to other hooks but librtmp won't use it otherwise. This
     * function is meant for allocating plugin's private data
     * structures and setting callbacks (using
     * RTMP_AttachCallback()). */
  void (*delete)(RTMP *r, void *data); 
    /* This is called called by RTMP_Free(). The last parameter is the
     * value returned by the create hook. This should free the memory
     * allocated by the create and parseOption hooks. */
} RTMP_Plugin;

void RTMPPlugin_DeleteInstances(RTMP *r);
void RTMPPlugin_OptUsage(int loglevel);
int RTMPPlugin_InitializePluginAndOpt(RTMP *r, const AVal *opt, const AVal *arg);

#define RTMP_PLUGIN_REGISTER(p)          \
  const RTMP_Plugin *rtmp_init_plugin() \
    {                                    \
      return &(p);                       \
    };

#ifdef __cplusplus
};
#endif

#endif
