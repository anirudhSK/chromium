// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_web_ui.h"
#include "chrome/browser/extensions/test_extension_system.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/manifest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/login/user_manager.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#endif

namespace extensions {

class ExtensionWebUITest : public testing::Test {
 public:
  ExtensionWebUITest()
      : ui_thread_(content::BrowserThread::UI, &message_loop_) {}

 protected:
  virtual void SetUp() OVERRIDE {
    profile_.reset(new TestingProfile());
    TestExtensionSystem* system =
        static_cast<TestExtensionSystem*>(ExtensionSystem::Get(profile_.get()));
    extension_service_ = system->CreateExtensionService(
        CommandLine::ForCurrentProcess(), base::FilePath(), false);
  }

  virtual void TearDown() OVERRIDE {
    profile_.reset();
    message_loop_.RunUntilIdle();
  }

  scoped_ptr<TestingProfile> profile_;
  ExtensionService* extension_service_;
  base::MessageLoop message_loop_;
  content::TestBrowserThread ui_thread_;

#if defined OS_CHROMEOS
  chromeos::ScopedTestDeviceSettingsService test_device_settings_service_;
  chromeos::ScopedTestCrosSettings test_cros_settings_;
  chromeos::ScopedTestUserManager test_user_manager_;
#endif
};

// Test that component extension url overrides have lower priority than
// non-component extension url overrides.
TEST_F(ExtensionWebUITest, ExtensionURLOverride) {
  // Make a component extension.
  extensions::DictionaryBuilder manifest;
  manifest.Set(manifest_keys::kName, "ext1")
      .Set(manifest_keys::kVersion, "0.1")
      .Set(std::string(manifest_keys::kChromeURLOverrides),
           extensions::DictionaryBuilder().Set("bookmarks", "1.html"));
  scoped_refptr<Extension> ext_component(
      extensions::ExtensionBuilder()
          .SetManifest(manifest)
          .SetLocation(Manifest::COMPONENT)
          .SetID("abcdefghijabcdefghijabcdefghijaa")
          .Build());
  profile_->GetExtensionService()->AddComponentExtension(ext_component.get());

  // Make a non-component extension.
  extensions::DictionaryBuilder manifest2;
  manifest2.Set(manifest_keys::kName, "ext2")
      .Set(manifest_keys::kVersion, "0.1")
      .Set(std::string(manifest_keys::kChromeURLOverrides),
           extensions::DictionaryBuilder().Set("bookmarks", "2.html"));
  scoped_refptr<Extension> ext_unpacked(
      extensions::ExtensionBuilder()
          .SetManifest(manifest2)
          .SetLocation(Manifest::UNPACKED)
          .SetID("bbabcdefghijabcdefghijabcdefghij")
          .Build());
  profile_->GetExtensionService()->AddExtension(ext_unpacked.get());

  GURL expected_component_override_url(
      std::string(ext_component->url().spec()) + "1.html");
  GURL expected_unpacked_override_url(std::string(ext_unpacked->url().spec()) +
                                      "2.html");

  // Register non-component extension.
  ExtensionWebUI::RegisterChromeURLOverrides(
      profile_.get(), URLOverrides::GetChromeURLOverrides(ext_unpacked.get()));
  GURL url("chrome://bookmarks");
  EXPECT_TRUE(ExtensionWebUI::HandleChromeURLOverride(&url, profile_.get()));
  EXPECT_EQ(url, expected_unpacked_override_url);

  // Register component extension. Despite being registered more recently, the
  // non-component extension should still have precedence.
  ExtensionWebUI::RegisterChromeURLOverrides(
      profile_.get(), URLOverrides::GetChromeURLOverrides(ext_component.get()));
  url = GURL("chrome://bookmarks");
  EXPECT_TRUE(ExtensionWebUI::HandleChromeURLOverride(&url, profile_.get()));
  EXPECT_EQ(url, expected_unpacked_override_url);

  // Unregister non-component extension. Only component extension remaining.
  ExtensionWebUI::UnregisterChromeURLOverrides(
      profile_.get(), URLOverrides::GetChromeURLOverrides(ext_unpacked.get()));
  url = GURL("chrome://bookmarks");
  EXPECT_TRUE(ExtensionWebUI::HandleChromeURLOverride(&url, profile_.get()));
  EXPECT_EQ(url, expected_component_override_url);

  // This time the non-component extension was registered more recently.
  // The non-component extension should still have precedence.
  ExtensionWebUI::RegisterChromeURLOverrides(
      profile_.get(), URLOverrides::GetChromeURLOverrides(ext_unpacked.get()));
  url = GURL("chrome://bookmarks");
  EXPECT_TRUE(ExtensionWebUI::HandleChromeURLOverride(&url, profile_.get()));
  EXPECT_EQ(url, expected_unpacked_override_url);
}

}  // namespace extensions
