/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2025  Christian Fillion
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
#  include <commctrl.h>
#  include <richedit.h>
#else
#  define PROGRESS_CLASS "msctls_progress32"
#  define WC_LISTVIEW "SysListView32"
#  define WC_TABCONTROL "SysTabControl32"
#  define PBS_MARQUEE 0
#endif

#define DIALOG_STYLE DS_MODALFRAME | WS_POPUP | WS_SYSMENU | WS_CAPTION
#define DIALOG_FONT  8, "MS Shell Dlg"

#define IDAPPLY 0x3021

#define IDD_PROGRESS_DIALOG 100
#define IDD_REPORT_DIALOG   101
#define IDD_CONFIG_DIALOG   102
#define IDD_ABOUT_DIALOG    103
#define IDD_IMPORT_DIALOG   104
#define IDD_BROWSER_DIALOG  105
#define IDD_NETCONF_DIALOG  106
#define IDD_OBSQUERY_DIALOG 107

#define IDC_LABEL      200
#define IDC_LABEL2     201
#define IDC_LABEL3     202
#define IDC_PROGRESS   210
#define IDC_REPORT     211
#define IDC_LIST       212
#define IDC_IMPORT     213
#define IDC_TABS       214
#define IDC_ACTION     215
#define IDC_WEBSITE    216
#define IDC_DONATE     217
#define IDC_ABOUT      218
#define IDC_MENU       219
#define IDC_GROUPBOX   220
#define IDC_URL        221
#define IDC_FILTER     222
#define IDC_CLEAR      223
#define IDC_DISPLAY    224
#define IDC_SELECT     225
#define IDC_UNSELECT   226
#define IDC_OPTIONS    227
#define IDC_BROWSE     228
#define IDC_PROXY      229
#define IDC_VERIFYPEER 230
#define IDC_SCREENSHOT 231
#define IDC_ENABLE     232
#define IDC_CHANGELOG  233
#define IDC_DISCOVER   234
#define IDC_STALETHRSH 235
#define IDC_FALLBCKPXY 236

#endif
