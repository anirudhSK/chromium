// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/chrome_signin_delegate.h"

#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/signin/signin_promo.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/browser/ui/host_desktop.h"
#include "chrome/browser/ui/scoped_tabbed_browser_displayer.h"
#include "chrome/common/extensions/extension_constants.h"
#include "content/public/common/page_transition_types.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

#if !defined(OS_CHROMEOS)
SigninManagerBase* GetSigninManager(Profile* profile) {
  return SigninManagerFactory::GetForProfile(profile);
}
#endif  // !defined(OS_CHROMEOS)

}  // namespace

ChromeSigninDelegate::ChromeSigninDelegate() {}

ChromeSigninDelegate::~ChromeSigninDelegate() {}

void ChromeSigninDelegate::SetProfile(Profile* profile) {
  profile_ = profile;
}

bool ChromeSigninDelegate::NeedSignin()  {
#if defined(OS_CHROMEOS)
  return false;
#else
  if (!profile_)
    return false;

  if (!GetSigninManager(profile_))
    return false;

  return GetSigninManager(profile_)->GetAuthenticatedUsername().empty();
#endif
}

void ChromeSigninDelegate::ShowSignin() {
  DCHECK(profile_);
  chrome::ScopedTabbedBrowserDisplayer displayer(
      profile_, chrome::GetActiveDesktop());
  chrome::ShowBrowserSignin(displayer.browser(), signin::SOURCE_APP_LAUNCHER);
}

void ChromeSigninDelegate::OpenLearnMore() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  GURL gurl(rb.GetLocalizedString(IDS_APP_LIST_SIGNIN_LEARN_MORE_LINK));
  chrome::NavigateParams params(profile_, gurl, content::PAGE_TRANSITION_LINK);
  chrome::Navigate(&params);
}

void ChromeSigninDelegate::OpenSettings() {
  ExtensionService* service = profile_->GetExtensionService();
  DCHECK(service);
  const extensions::Extension* extension = service->GetInstalledExtension(
      extension_misc::kSettingsAppId);
  if (!extension)
    return;

  OpenApplication(AppLaunchParams(profile_, extension, NEW_FOREGROUND_TAB));
}

base::string16 ChromeSigninDelegate::GetSigninHeading() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  return rb.GetLocalizedString(IDS_APP_LIST_SIGNIN_HEADING);
}

base::string16 ChromeSigninDelegate::GetSigninText() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  return rb.GetLocalizedString(IDS_APP_LIST_SIGNIN_TEXT);
}

base::string16 ChromeSigninDelegate::GetSigninButtonText() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  return rb.GetLocalizedString(IDS_APP_LIST_SIGNIN_BUTTON);
}

base::string16 ChromeSigninDelegate::GetLearnMoreLinkText() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  return rb.GetLocalizedString(IDS_APP_LIST_SIGNIN_LEARN_MORE_TEXT);
}

base::string16 ChromeSigninDelegate::GetSettingsLinkText() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  return rb.GetLocalizedString(IDS_APP_LIST_SIGNIN_SETTINGS_TEXT);
}
