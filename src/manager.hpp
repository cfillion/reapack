/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2023  Christian Fillion
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

#ifndef REAPACK_MANAGER_HPP
#define REAPACK_MANAGER_HPP

#include "dialog.hpp"

#include <boost/logic/tribool.hpp>
#include <map>
#include <optional>

class ListView;
class Menu;
class Remote;
struct NetworkOpts;

typedef boost::logic::tribool tribool;

class Manager : public Dialog {
public:
  Manager();

  void refresh();
  bool importRepo();

protected:
  void onInit() override;
  void onCommand(int, int) override;
  bool onKeyDown(int, int) override;
  void onTimer(int) override;
  void onClose() override;

private:
  struct RemoteMods {
    std::optional<bool> enable;
    std::optional<tribool> autoInstall;
    operator bool() const { return enable || autoInstall; }
  };

  typedef std::function<void (const Remote &, int index, RemoteMods *)> ModsCallback;

  Remote getRemote(int index) const;
  bool fillContextMenu(Menu &, int index) const;
  void setMods(const ModsCallback &);
  void toggleEnabled();
  bool isRemoteEnabled(const Remote &) const;
  void setRemoteAutoInstall(const tribool &);
  tribool remoteAutoInstall(const Remote &) const;
  void uninstall();
  void toggle(std::optional<bool> &, bool current);
  void refreshIndex();
  void copyUrl();
  void launchBrowser();
  void importExport();
  void options();
  void setupNetwork();
  void importArchive();
  void exportArchive();
  void aboutRepo(bool focus = true);

  void setChange(int);
  bool confirm() const;
  bool apply();
  void reset();

  HWND m_apply;
  ListView *m_list;

  size_t m_changes;
  bool m_importing;

  std::map<Remote, RemoteMods> m_mods;
  std::set<Remote> m_uninstall;
  std::optional<bool> m_autoInstall, m_bleedingEdge,
                      m_promptObsolete, m_expandSynonyms;

  Serializer m_serializer;
};

class NetworkConfig : public Dialog {
public:
  NetworkConfig(NetworkOpts *);

protected:
  void onInit() override;
  void onCommand(int, int) override;

private:
  void apply();

  NetworkOpts *m_opts;
  HWND m_proxy;
  HWND m_staleThreshold;
  HWND m_verifyPeer;
};

#endif
