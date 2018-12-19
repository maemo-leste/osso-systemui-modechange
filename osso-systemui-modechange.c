/**
   @file osso-systemui-modechange.c

   @brief Maemo systemui modechange dialog plugin

   Copyright (C) 2015 Jonathan Wilson <jfwfreo@tpgi.com.au>

   This file is part of osso-systemui-modechange.

   this library is free software;
   you can redistribute it and/or modify it under the terms of the
   GNU Lesser General Public License version 2.1 as published by the
   Free Software Foundation.

   osso-systemui-modechange is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with osso-systemui-modechange.
   If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#include <locale.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gconf/gconf-client.h>
#include <pango/pango.h>
#include <dbus/dbus.h>
#include <hildon/hildon-note.h>
#include <systemui.h>
#include <systemui/modechange-dbus-names.h>

typedef struct {
  GtkWidget *note;
  system_ui_callback_t callback;
  system_ui_data *ui;
  int window_priority;
} modechange_priv_t;

modechange_priv_t priv;

int
modechange_close_handler(const char *interface, const char *method,
                         GArray *args, system_ui_data *data,
                         system_ui_handler_arg *result)
{
  WindowPriority_HideWindow(priv.note);

  if (priv.note)
  {
    gtk_object_destroy(GTK_OBJECT(priv.note));
    priv.note = NULL;
  }

  systemui_free_callback(&priv.callback);

  return DBUS_TYPE_VARIANT;
}

void
modechange_destroy_window(guint argc, system_ui_data *data)
{
  if (priv.note)
  {
    do_callback(data, &priv.callback, argc);
    WindowPriority_HideWindow(priv.note);
    gtk_object_destroy(GTK_OBJECT(priv.note));
    priv.note = NULL;
    systemui_free_callback(&priv.callback);
  }
}

void
modechange_response_handler(GtkWidget *dialog, gint response_id,
                            system_ui_data *data)
{
  if (response_id == GTK_RESPONSE_OK)
    modechange_destroy_window(1, data);
  else if (response_id == GTK_RESPONSE_CANCEL)
    modechange_destroy_window(2, data);
}

void
modechange_destroy_handler(GtkObject *object,system_ui_data *data)
{
  if (priv.note)
    modechange_destroy_window(2, data);
}

gboolean
modechange_key_press_event_handler(GtkWidget *widget, GdkEventKey *event,
                                   system_ui_data *data)
{
  if (event->keyval == XK_Escape)
  {
    modechange_response_handler(GTK_WIDGET(priv.note), GTK_RESPONSE_CANCEL,
                                data);
    return TRUE;
  }

  return FALSE;
}

int
modechange_open_handler(const char *interface,
                        const char *method,
                        GArray *args,
                        system_ui_data *data,
                        system_ui_handler_arg *result)
{
  int supported_args[1] = {'u'};
  system_ui_handler_arg *hargs;
  modechange_t mode;

  if(!check_plugin_arguments(args, supported_args, 1))
    return FALSE;

  hargs = ((system_ui_handler_arg *)args->data);
  mode = hargs[4].data.u32;

  switch (mode)
  {
    case MODECHANGE_TO_FLIGHTMODE:
    {

      systemui_do_callback(data, &priv.callback, 1);
      systemui_free_callback(&priv.callback);
      break;
    }
    case MODECHANGE_TO_NORMALMODE:
    {
      if (priv.note)
      {
        WindowPriority_HideWindow(priv.note);
        gtk_object_destroy(GTK_OBJECT(priv.note));
      }

      priv.note =
          hildon_note_new_confirmation(GTK_WINDOW(data->parent),
                                       dgettext("osso-powerup-shutdown",
                                                "powerup_nc_exit_flight_mode"));
      g_signal_connect(G_OBJECT(priv.note), "response",
                       G_CALLBACK(modechange_response_handler), data);
      g_signal_connect(G_OBJECT(priv.note), "key-press-event",
                       G_CALLBACK(modechange_key_press_event_handler),data);
      g_signal_connect(G_OBJECT(priv.note), "destroy",
                       G_CALLBACK(modechange_destroy_handler), data);
      WindowPriority_ShowWindow(priv.note,priv.window_priority);

      GtkWidget *grab = gtk_grab_get_current();

      if (grab)
        gtk_grab_remove(grab);

      break;
    }
    default:
    {
      if (priv.note)
      {
        WindowPriority_HideWindow(priv.note);
        gtk_object_destroy(GTK_OBJECT(priv.note));
        return DBUS_TYPE_INVALID;
      }

      break;
    }
  }

  if (!check_set_callback(args, &priv.callback))
    result->data.i32 = -2;
  else
    result->data.i32 = -3;

  return DBUS_TYPE_INT32;
}

void
plugin_close(system_ui_data *data)
{
  remove_handler(SYSTEMUI_MODECHANGE_OPEN_REQ,data);
  remove_handler(SYSTEMUI_MODECHANGE_CLOSE_REQ,data);
  WindowPriority_HideWindow(priv.note);

  if (priv.note)
  {
    gtk_object_destroy(GTK_OBJECT(priv.note));
    priv.note = NULL;
  }

  systemui_free_callback(&priv.callback);
}

gboolean
plugin_init(system_ui_data *data)
{
  priv.ui = data;
  priv.note = NULL;
  priv.window_priority = gconf_client_get_int(
        data->gc_client, "/system/systemui/modechange/window_priority", NULL);

  if (!priv.window_priority)
    priv.window_priority = 60;

  add_handler(SYSTEMUI_MODECHANGE_OPEN_REQ, modechange_open_handler, data);
  add_handler(SYSTEMUI_MODECHANGE_CLOSE_REQ, modechange_close_handler, data);

  return TRUE;
}
