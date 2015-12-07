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
