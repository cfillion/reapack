/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2018  Christian Fillion
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
#include <memory>
#include <optional>

class ListView;
class Menu;
struct NetworkOpts;

class Remote;
typedef std::shared_ptr<Remote> RemotePtr;

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
    RemotePtr remote;

    std::optional<bool> enable;
    std::optional<boost::tribool> autoInstall;
    bool uninstall;

    operator bool() const { return enable || autoInstall || uninstall; }
    bool operator==(const RemotePtr &r) const { return remote == r; }
  };

  typedef std::function<void (RemoteMods &, int index)> ModsCallback;

  auto findMods(const RemotePtr &r) { return std::find(m_mods.begin(), m_mods.end(), r); }
  auto findMods(const RemotePtr &r) const { return std::find(m_mods.begin(), m_mods.end(), r); }
  void setMods(const ModsCallback &);
  void setMods(int index, const ModsCallback &);

  RemotePtr getRemote(int index) const;

  bool fillContextMenu(Menu &, int index) const;
  void toggleEnabled();
  bool isRemoteEnabled(const RemotePtr &) const;
  void setRemoteAutoInstall(const boost::tribool &);
  boost::tribool remoteAutoInstall(const RemotePtr &) const;
  void uninstall();
  bool isUninstalled(const RemotePtr &) const;
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

  std::vector<RemoteMods> m_mods;
  std::optional<bool> m_autoInstall;
  std::optional<bool> m_bleedingEdge;
  std::optional<bool> m_promptObsolete;

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
