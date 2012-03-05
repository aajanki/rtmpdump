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

#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ltdl.h>
#include <stdlib.h>
#include "plugin.h"
#include "log.h"

#define SO_EXT_WITH_DOT "." SO_EXT

typedef struct RTMPPluginNode
{
  struct RTMPPluginNode *next;
  RTMP_Plugin *plugin;
} RTMPPluginNode;

typedef struct RTMPPluginInstance
{
  struct RTMPPluginInstance *next;
  RTMP_Plugin *plugin;
  void *userdata;
} RTMPPluginInstance;

typedef struct RTMPPluginManager
{
  RTMPPluginNode *plugins;
  int initialized;
} RTMPPluginManager;

static RTMPPluginManager pluginManager = { NULL, FALSE };

static RTMPPluginNode *GetPlugins(void);
static void LoadPlugins(const char *plugindir);
static RTMP_Plugin *LoadPlugin(const char *path);
static RTMPPluginInstance *CreatePluginInstance(RTMP *r, RTMP_Plugin *plugin);
static int AddPlugin(RTMPPluginManager *manager, RTMP_Plugin *plugin,
                     const char *filename);
static RTMP_Plugin *FindPluginByOpt(const AVal *opt,
                                    const RTMPPluginOption **optinfo);
void InitializePlugins(void);

static RTMPPluginNode *GetPlugins(void)
{
  if (!pluginManager.initialized)
    InitializePlugins();
  return pluginManager.plugins;
}

void InitializePlugins(void)
{
  if (lt_dlinit() != 0)
    {
      RTMP_Log(RTMP_LOGERROR, "Failed to load plugins");
      return;
    }

  const char *home = getenv("HOME");
  if (home)
    {
      char *homeplugindir = ".librtmp/plugins";
      size_t len = strlen(home)+strlen(homeplugindir)+2;
      char *buf = malloc(len);
      if (buf)
        {
          snprintf(buf, len, "%s/%s", home, homeplugindir);
          LoadPlugins(buf);
          free(buf);
        }
    }

  LoadPlugins(PLUGINDIR);

  pluginManager.initialized = TRUE;
}

static void LoadPlugins(const char *plugindir)
{
  DIR *dir;
  struct dirent *de;
  char *fullname;
  size_t len;
  RTMP_Plugin *plugin;

  RTMP_Log(RTMP_LOGDEBUG, "Loading plugins from %s", plugindir);

  dir = opendir(plugindir);
  if (!dir)
    {
      RTMP_Log(RTMP_LOGDEBUG, "Can not read plugin dir %s: %s",
               plugindir, strerror(errno));
      return;
    }

  while ((de = readdir(dir)))
    {
      if (strcmp(de->d_name + strlen(de->d_name) - strlen(SO_EXT_WITH_DOT), SO_EXT_WITH_DOT) == 0)
        {
          len = strlen(plugindir) + strlen(de->d_name) + 2;
          fullname = malloc(len);
          snprintf(fullname, len, "%s/%s", plugindir, de->d_name);
          plugin = LoadPlugin(fullname);
          if (plugin)
            AddPlugin(&pluginManager, plugin, fullname);
          free(fullname);
        }
    }

  closedir(dir);
}

static RTMP_Plugin *LoadPlugin(const char *filename)
{
  lt_dlhandle dl;
  RTMP_Plugin *(*initializer)(void);
  RTMP_Plugin *plugin = NULL;

  RTMP_Log(RTMP_LOGDEBUG, "Loading plugin %s", filename);
  
  dl = lt_dlopen(filename);
  if (!dl)
    {
      RTMP_Log(RTMP_LOGERROR, "Failed to open plugin %s: %s",
               filename, lt_dlerror());
      return NULL;
    }

  initializer = (RTMP_Plugin *(*)(void))lt_dlsym(dl, "rtmp_init_plugin");
  if (!initializer)
    {
      RTMP_Log(RTMP_LOGERROR, "Plugin %s has no initializer: %s",
               filename, lt_dlerror());
      goto abandon;
    }

  plugin = initializer();
  if (!plugin)
    {
      RTMP_Log(RTMP_LOGERROR, "Plugin %s initializer failed", filename);
      goto abandon;
    }

  if (plugin->requiredAPIVersion != RTMP_PLUGIN_API)
    {
      RTMP_Log(RTMP_LOGERROR, "Incompatible API version on plugin %s (plugin: %d, librtmp: %d)",
               filename, plugin->requiredAPIVersion, RTMP_PLUGIN_API);
      goto abandon;
    }

  return plugin;

abandon:
  lt_dlclose(dl);
  return NULL;
}

static int AddPlugin(RTMPPluginManager *manager, RTMP_Plugin *plugin,
                     const char *filename)
{
  RTMPPluginNode *node, *pl;

  node = malloc(sizeof(RTMPPluginNode));
  if (!node)
    return FALSE;
  node->next = NULL;
  node->plugin = plugin;

  if (manager->plugins)
    {
      pl = manager->plugins;

      while (1)
        {
          /* Has this plugin already been loaded? */
          if (AVMATCH(&pl->plugin->name, &plugin->name))
            {
              RTMP_Log(RTMP_LOGWARNING, "Trying to load duplicate plugin %.*s %.*s from %s",
                       plugin->name.av_len, plugin->name.av_val,
                       plugin->version.av_len, plugin->version.av_val,
                       filename);
              return TRUE;
            }

          if (!pl->next)
            break;

          pl = pl->next;
        }

      pl->next = node;
    }
  else
    {
      manager->plugins = node;
    }

  return TRUE;
}

void RTMPPlugin_DeleteInstances(RTMP *r)
{
  RTMPPluginInstance *node, *next;

  node = r->pluginInstances;
  while (node)
    {
      if (node->plugin && node->plugin->delete)
        node->plugin->delete(r, node->userdata);
      next = node->next;
      free(node);
      node = next;
    }

  r->pluginInstances = NULL;
}

void RTMPPlugin_OptUsage(int loglevel)
{
  RTMPPluginNode *node;

  node = GetPlugins();
  while (node)
    {
      if (node->plugin->options)
        {
          RTMP_Log(loglevel, "Options provided by %.*s %.*s plugin:\n",
                   node->plugin->name.av_len, node->plugin->name.av_val,
                   node->plugin->version.av_len, node->plugin->version.av_val);

          RTMPPluginOption *opt = node->plugin->options;
          while (opt && opt->name.av_len)
            {
              RTMP_Log(loglevel, "%10s %-7s  %s\n",
                       opt->name.av_val, opt->type.av_val, opt->usage.av_val);
              opt++;
            }
        }
      node = node->next;
    }
}

static RTMP_Plugin *FindPluginByOpt(const AVal *opt,
                                    const RTMPPluginOption **optinfo)
{
  RTMPPluginNode *pl = GetPlugins();
  while (pl)
    {
      const RTMPPluginOption *plopt = pl->plugin->options;
      while (plopt && plopt->name.av_len)
        {
          if (opt->av_len == plopt->name.av_len && 
              !strcasecmp(opt->av_val, plopt->name.av_val))
            {
              *optinfo = plopt;
              return pl->plugin;
            }

          plopt++;
        }

      pl = pl->next;
    }

  return NULL;
}

static RTMPPluginInstance *CreatePluginInstance(RTMP *r, RTMP_Plugin *plugin)
{
  RTMPPluginInstance *instance, *node;

  RTMP_Log(RTMP_LOGDEBUG, "Creating new plugin instance: %.*s",
           plugin->name.av_len, plugin->name.av_val);

  instance = malloc(sizeof(RTMPPluginInstance));
  if (!instance)
    return NULL;
  instance->next = NULL;
  instance->plugin = plugin;
  if (plugin->create)
    instance->userdata = plugin->create(r);
  else
    instance->userdata = NULL;

  if (!r->pluginInstances)
    {
      r->pluginInstances = instance;
    }
  else
    {
      node = r->pluginInstances;
      while (node->next)
        node = node->next;
      node->next = instance;
    }

  return instance;
}

int RTMPPlugin_InitializePluginAndOpt(RTMP *r, const AVal *opt, const AVal *arg)
{
  RTMPPluginInstance *instance, *node;
  const RTMPPluginOption *optinfo;

  RTMP_Plugin *plugin = FindPluginByOpt(opt, &optinfo);
  if (!plugin)
    return FALSE;

  RTMP_Log(RTMP_LOGDEBUG, "Plugin %.*s will handle opt %.*s",
           plugin->name.av_len, plugin->name.av_val,
           opt->av_len, opt->av_val);

  /* Does r already have instance of the plugin? */
  instance = NULL;
  node = r->pluginInstances;
  while (node)
    {
      if (node->plugin == plugin)
        {
          instance = node;
          break;
        }

      node = node->next;
    }

  /* If not, create a new instance now */
  if (!instance)
    {
      instance = CreatePluginInstance(r, plugin);
      if (!instance)
        return FALSE;
    }

  /* Process the option value */
  if (optinfo->parseOption)
    optinfo->parseOption(opt, arg, instance->userdata);

  return TRUE;
}
