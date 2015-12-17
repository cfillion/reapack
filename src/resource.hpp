/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015  Christian Fillion
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

#ifndef _WIN32
#define PROGRESS_CLASS "msctls_progress32"
#else
#include <commctrl.h>
#endif

#define IDD_PROGRESS_DIALOG 100
#define IDD_REPORT_DIALOG 101

#define IDC_LABEL 200
#define IDC_PROGRESS 201
#define IDC_REPORT 202

#endif
