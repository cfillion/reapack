/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2016  Christian Fillion
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef REAPACK_RESOURCE_HPP
#define REAPACK_RESOURCE_HPP

#ifdef _WIN32
#include <commctrl.h>
#include <richedit.h>
#else
#define PROGRESS_CLASS "msctls_progress32"
#define WC_LISTVIEW "SysListView32"
#define WC_TABCONTROL "SysTabControl32"
#endif

#define DIALOG_STYLE \
  DS_MODALFRAME | DS_SHELLFONT | WS_POPUP | WS_SYSMENU | WS_CAPTION
#define DIALOG_FONT 8, "MS Shell Dlg"

#define IDD_PROGRESS_DIALOG 100
#define IDD_REPORT_DIALOG   101
#define IDD_CONFIG_DIALOG   102
#define IDD_ABOUT_DIALOG    103

#define IDC_LABEL      200
#define IDC_PROGRESS   201
#define IDC_REPORT     202
#define IDC_LIST       203
#define IDC_IMPORT     204
#define IDC_TABS       205
#define IDC_ENABLE     206
#define IDC_WEBSITE    207
#define IDC_DONATE     208
#define IDC_ABOUT      209
#define IDC_CATEGORIES 210
#define IDC_PACKAGES   211

#endif
