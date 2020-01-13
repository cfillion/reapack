#include "listview.hpp"

#include <Cocoa/Cocoa.h>

// NSTableView::selectRowIndexes is an expansive operation so selecting
// thousands of items individually can take seconds on macOS
// (observed on 10.10 and 10.15).
//
// As a side effect of the optimizations below, SWELL will post a single
// LVN_ITEMCHANGED notification instead of one per item.

void ListView::setSelected(const std::vector<int> &indexes, const bool select)
{
  auto indexSet = [NSMutableIndexSet indexSet];

  for(const int index : indexes)
    [indexSet addIndex: index];

  auto tableView = reinterpret_cast<NSTableView *>(handle());
  [tableView selectRowIndexes: indexSet byExtendingSelection: select];
}

void ListView::selectAll()
{
  auto tableView = reinterpret_cast<NSTableView *>(handle());
  [tableView selectAll: tableView];
}

void ListView::unselectAll()
{
  auto tableView = reinterpret_cast<NSTableView *>(handle());
  [tableView deselectAll: tableView];
}
