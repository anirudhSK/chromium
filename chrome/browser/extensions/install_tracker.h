// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_INSTALL_TRACKER_H_
#define CHROME_BROWSER_EXTENSIONS_INSTALL_TRACKER_H_

#include "base/observer_list.h"
#include "base/prefs/pref_change_registrar.h"
#include "chrome/browser/extensions/install_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class Profile;

namespace extensions {

class ExtensionPrefs;

class InstallTracker : public KeyedService,
                       public content::NotificationObserver {
 public:
  InstallTracker(Profile* profile,
                 extensions::ExtensionPrefs* prefs);
  virtual ~InstallTracker();

  void AddObserver(InstallObserver* observer);
  void RemoveObserver(InstallObserver* observer);

  void OnBeginExtensionInstall(
      const InstallObserver::ExtensionInstallParams& params);
  void OnDownloadProgress(const std::string& extension_id,
                          int percent_downloaded);
  void OnInstallFailure(const std::string& extension_id);

  // Overriddes for KeyedService:
  virtual void Shutdown() OVERRIDE;

  // content::NotificationObserver
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

 private:
  void OnAppsReordered();

  ObserverList<InstallObserver> observers_;
  content::NotificationRegistrar registrar_;
  PrefChangeRegistrar pref_change_registrar_;

  DISALLOW_COPY_AND_ASSIGN(InstallTracker);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_INSTALL_TRACKER_H_
