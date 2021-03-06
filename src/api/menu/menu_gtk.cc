// Copyright (c) 2012 Intel Corp
// Copyright (c) 2012 The Chromium Authors
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell co
// pies of the Software, and to permit persons to whom the Software is furnished
//  to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in al
// l copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IM
// PLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNES
// S FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
//  OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WH
// ETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "content/nw/src/api/menu/menu.h"

#include "base/values.h"
#include "base/logging.h"
#include "content/nw/src/api/menuitem/menuitem.h"
#include "content/nw/src/nw_shell.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/render_widget_host_view.h"
#include "ui/gfx/point.h"

namespace api {

namespace {

// Popup menus may get squished if they open up too close to the bottom of the
// screen. This function takes the size of the screen, the size of the menu,
// an optional widget, the Y position of the mouse click, and adjusts the popup
// menu's Y position to make it fit if it's possible to do so.
// Returns the new Y position of the popup menu.
int CalculateMenuYPosition(const GdkRectangle* screen_rect,
                           const GtkRequisition* menu_req,
                           GtkWidget* widget, const int y) {
  CHECK(screen_rect);
  CHECK(menu_req);
  // If the menu would run off the bottom of the screen, and there is enough
  // screen space upwards to accommodate the menu, then pop upwards. If there
  // is a widget, then also move the anchor point to the top of the widget
  // rather than the bottom.
  const int screen_top = screen_rect->y;
  const int screen_bottom = screen_rect->y + screen_rect->height;
  const int menu_bottom = y + menu_req->height;
  int alternate_y = y - menu_req->height;
  if (widget) {
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    alternate_y -= allocation.height;
  }
  if (menu_bottom >= screen_bottom && alternate_y >= screen_top)
    return alternate_y;
  return y;
}

void PointMenuPositionFunc(GtkMenu* menu,
                           int* x,
                           int* y,
                           gboolean* push_in,
                           gpointer userdata) {
  *push_in = TRUE;

  gfx::Point* point = reinterpret_cast<gfx::Point*>(userdata);
  *x = point->x();
  *y = point->y();

  GtkRequisition menu_req;
  gtk_widget_size_request(GTK_WIDGET(menu), &menu_req);
  GdkScreen* screen;
  gdk_display_get_pointer(gdk_display_get_default(), &screen, NULL, NULL, NULL);
  gint monitor = gdk_screen_get_monitor_at_point(screen, *x, *y);

  GdkRectangle screen_rect;
  gdk_screen_get_monitor_geometry(screen, monitor, &screen_rect);

  *y = CalculateMenuYPosition(&screen_rect, &menu_req, NULL, *y);
}

}  // namespace

void Menu::Create(const base::DictionaryValue& option) {
  menu_ = gtk_menu_new();
  gtk_widget_show(menu_);
  g_object_ref_sink(G_OBJECT(menu_));
}

void Menu::Destroy() {
  gtk_widget_destroy(menu_);
  g_object_unref(G_OBJECT(menu_));
}

void Menu::Append(MenuItem* menu_item) {
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), menu_item->menu_item_);
}

void Menu::Insert(MenuItem* menu_item, int pos) {
  gtk_menu_shell_insert(GTK_MENU_SHELL(menu_), menu_item->menu_item_, pos);
}

void Menu::Remove(MenuItem* menu_item, int pos) {
  gtk_container_remove(GTK_CONTAINER(menu_), menu_item->menu_item_);
}

void Menu::Popup(int x, int y, content::Shell* shell) {
  GdkEventButton* event = shell->web_contents()->GetRenderWidgetHostView()->
      GetLastMouseDown();
  uint32_t triggering_event_time = event ? event->time : GDK_CURRENT_TIME;
  gfx::Point point(event->x_root, event->y_root);
  gtk_menu_popup(GTK_MENU(menu_), NULL, NULL, 
                 PointMenuPositionFunc, &point,
                 3, triggering_event_time);
}

}  // namespace api
