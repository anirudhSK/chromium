// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/file_util.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/scoped_ptr.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension_l10n_util.h"
#include "chrome/common/extensions/message_bundle.h"
#include "extensions/common/constants.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/manifest_constants.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using extensions::kLocaleFolder;
using extensions::kMessagesFilename;
using extensions::MessageBundle;

namespace errors = extensions::manifest_errors;
namespace keys = extensions::manifest_keys;

namespace {

TEST(ExtensionL10nUtil, ValidateLocalesWithBadLocale) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());

  base::FilePath src_path = temp.path().Append(kLocaleFolder);
  base::FilePath locale = src_path.AppendASCII("ms");
  ASSERT_TRUE(base::CreateDirectory(locale));

  base::FilePath messages_file = locale.Append(kMessagesFilename);
  std::string data = "{ \"name\":";
  ASSERT_TRUE(base::WriteFile(messages_file, data.c_str(), data.length()));

  base::DictionaryValue manifest;
  manifest.SetString(keys::kDefaultLocale, "en");
  std::string error;
  EXPECT_FALSE(extension_l10n_util::ValidateExtensionLocales(
      temp.path(), &manifest, &error));
  EXPECT_THAT(error,
              testing::HasSubstr(
                  base::UTF16ToUTF8(messages_file.LossyDisplayName())));
}

TEST(ExtensionL10nUtil, GetValidLocalesEmptyLocaleFolder) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());

  base::FilePath src_path = temp.path().Append(kLocaleFolder);
  ASSERT_TRUE(base::CreateDirectory(src_path));

  std::string error;
  std::set<std::string> locales;
  EXPECT_FALSE(extension_l10n_util::GetValidLocales(src_path,
                                                    &locales,
                                                    &error));

  EXPECT_TRUE(locales.empty());
}

TEST(ExtensionL10nUtil, GetValidLocalesWithValidLocaleNoMessagesFile) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());

  base::FilePath src_path = temp.path().Append(kLocaleFolder);
  ASSERT_TRUE(base::CreateDirectory(src_path));
  ASSERT_TRUE(base::CreateDirectory(src_path.AppendASCII("sr")));

  std::string error;
  std::set<std::string> locales;
  EXPECT_FALSE(extension_l10n_util::GetValidLocales(src_path,
                                                    &locales,
                                                    &error));

  EXPECT_TRUE(locales.empty());
}

TEST(ExtensionL10nUtil, GetValidLocalesWithUnsupportedLocale) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());

  base::FilePath src_path = temp.path().Append(kLocaleFolder);
  ASSERT_TRUE(base::CreateDirectory(src_path));
  // Supported locale.
  base::FilePath locale_1 = src_path.AppendASCII("sr");
  ASSERT_TRUE(base::CreateDirectory(locale_1));
  std::string data("whatever");
  ASSERT_TRUE(base::WriteFile(
      locale_1.Append(kMessagesFilename),
      data.c_str(), data.length()));
  // Unsupported locale.
  ASSERT_TRUE(base::CreateDirectory(src_path.AppendASCII("xxx_yyy")));

  std::string error;
  std::set<std::string> locales;
  EXPECT_TRUE(extension_l10n_util::GetValidLocales(src_path,
                                                   &locales,
                                                   &error));

  EXPECT_FALSE(locales.empty());
  EXPECT_TRUE(locales.find("sr") != locales.end());
  EXPECT_FALSE(locales.find("xxx_yyy") != locales.end());
}

TEST(ExtensionL10nUtil, GetValidLocalesWithValidLocalesAndMessagesFile) {
  base::FilePath install_dir;
  ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &install_dir));
  install_dir = install_dir.AppendASCII("extensions")
      .AppendASCII("good")
      .AppendASCII("Extensions")
      .AppendASCII("behllobkkfkfnphdnhnkndlbkcpglgmj")
      .AppendASCII("1.0.0.0")
      .Append(kLocaleFolder);

  std::string error;
  std::set<std::string> locales;
  EXPECT_TRUE(extension_l10n_util::GetValidLocales(install_dir,
                                                   &locales,
                                                   &error));
  EXPECT_EQ(3U, locales.size());
  EXPECT_TRUE(locales.find("sr") != locales.end());
  EXPECT_TRUE(locales.find("en") != locales.end());
  EXPECT_TRUE(locales.find("en_US") != locales.end());
}

TEST(ExtensionL10nUtil, LoadMessageCatalogsValidFallback) {
  base::FilePath install_dir;
  ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &install_dir));
  install_dir = install_dir.AppendASCII("extensions")
      .AppendASCII("good")
      .AppendASCII("Extensions")
      .AppendASCII("behllobkkfkfnphdnhnkndlbkcpglgmj")
      .AppendASCII("1.0.0.0")
      .Append(kLocaleFolder);

  std::string error;
  std::set<std::string> locales;
  EXPECT_TRUE(extension_l10n_util::GetValidLocales(install_dir,
                                                   &locales,
                                                   &error));

  scoped_ptr<MessageBundle> bundle(extension_l10n_util::LoadMessageCatalogs(
      install_dir, "sr", "en_US", locales, &error));
  ASSERT_FALSE(NULL == bundle.get());
  EXPECT_TRUE(error.empty());
  EXPECT_EQ("Color", bundle->GetL10nMessage("color"));
  EXPECT_EQ("Not in the US or GB.", bundle->GetL10nMessage("not_in_US_or_GB"));
}

TEST(ExtensionL10nUtil, LoadMessageCatalogsMissingFiles) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());

  base::FilePath src_path = temp.path().Append(kLocaleFolder);
  ASSERT_TRUE(base::CreateDirectory(src_path));

  std::set<std::string> valid_locales;
  valid_locales.insert("sr");
  valid_locales.insert("en");
  std::string error;
  EXPECT_TRUE(NULL == extension_l10n_util::LoadMessageCatalogs(src_path,
                                                               "en",
                                                               "sr",
                                                               valid_locales,
                                                               &error));
  EXPECT_FALSE(error.empty());
}

TEST(ExtensionL10nUtil, LoadMessageCatalogsBadJSONFormat) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());

  base::FilePath src_path = temp.path().Append(kLocaleFolder);
  ASSERT_TRUE(base::CreateDirectory(src_path));

  base::FilePath locale = src_path.AppendASCII("sr");
  ASSERT_TRUE(base::CreateDirectory(locale));

  std::string data = "{ \"name\":";
  base::FilePath messages_file = locale.Append(kMessagesFilename);
  ASSERT_TRUE(base::WriteFile(messages_file, data.c_str(), data.length()));

  std::set<std::string> valid_locales;
  valid_locales.insert("sr");
  valid_locales.insert("en_US");
  std::string error;
  EXPECT_TRUE(NULL == extension_l10n_util::LoadMessageCatalogs(src_path,
                                                               "en_US",
                                                               "sr",
                                                               valid_locales,
                                                               &error));
  EXPECT_EQ(
      extensions::ErrorUtils::FormatErrorMessage(
          errors::kLocalesInvalidLocale,
          base::UTF16ToUTF8(messages_file.LossyDisplayName()),
          "Line: 1, column: 10, Unexpected token."),
      error);
}

TEST(ExtensionL10nUtil, LoadMessageCatalogsDuplicateKeys) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());

  base::FilePath src_path = temp.path().Append(kLocaleFolder);
  ASSERT_TRUE(base::CreateDirectory(src_path));

  base::FilePath locale_1 = src_path.AppendASCII("en");
  ASSERT_TRUE(base::CreateDirectory(locale_1));

  std::string data =
    "{ \"name\": { \"message\": \"something\" }, "
    "\"name\": { \"message\": \"something else\" } }";
  ASSERT_TRUE(base::WriteFile(locale_1.Append(kMessagesFilename),
                              data.c_str(), data.length()));

  base::FilePath locale_2 = src_path.AppendASCII("sr");
  ASSERT_TRUE(base::CreateDirectory(locale_2));

  ASSERT_TRUE(base::WriteFile(locale_2.Append(kMessagesFilename),
                              data.c_str(), data.length()));

  std::set<std::string> valid_locales;
  valid_locales.insert("sr");
  valid_locales.insert("en");
  std::string error;
  // JSON parser hides duplicates. We are going to get only one key/value
  // pair at the end.
  scoped_ptr<MessageBundle> message_bundle(
      extension_l10n_util::LoadMessageCatalogs(src_path,
                                               "en",
                                               "sr",
                                               valid_locales,
                                               &error));
  EXPECT_TRUE(NULL != message_bundle.get());
  EXPECT_TRUE(error.empty());
}

// Caller owns the returned object.
MessageBundle* CreateManifestBundle() {
  linked_ptr<base::DictionaryValue> catalog(new base::DictionaryValue);

  base::DictionaryValue* name_tree = new base::DictionaryValue();
  name_tree->SetString("message", "name");
  catalog->Set("name", name_tree);

  base::DictionaryValue* short_name_tree = new base::DictionaryValue();
  short_name_tree->SetString("message", "short_name");
  catalog->Set("short_name", short_name_tree);

  base::DictionaryValue* description_tree = new base::DictionaryValue();
  description_tree->SetString("message", "description");
  catalog->Set("description", description_tree);

  base::DictionaryValue* action_title_tree = new base::DictionaryValue();
  action_title_tree->SetString("message", "action title");
  catalog->Set("title", action_title_tree);

  base::DictionaryValue* omnibox_keyword_tree = new base::DictionaryValue();
  omnibox_keyword_tree->SetString("message", "omnibox keyword");
  catalog->Set("omnibox_keyword", omnibox_keyword_tree);

  base::DictionaryValue* file_handler_title_tree = new base::DictionaryValue();
  file_handler_title_tree->SetString("message", "file handler title");
  catalog->Set("file_handler_title", file_handler_title_tree);

  base::DictionaryValue* launch_local_path_tree = new base::DictionaryValue();
  launch_local_path_tree->SetString("message", "main.html");
  catalog->Set("launch_local_path", launch_local_path_tree);

  base::DictionaryValue* launch_web_url_tree = new base::DictionaryValue();
  launch_web_url_tree->SetString("message", "http://www.google.com/");
  catalog->Set("launch_web_url", launch_web_url_tree);

  base::DictionaryValue* first_command_description_tree =
      new base::DictionaryValue();
  first_command_description_tree->SetString("message", "first command");
  catalog->Set("first_command_description", first_command_description_tree);

  base::DictionaryValue* second_command_description_tree =
      new base::DictionaryValue();
  second_command_description_tree->SetString("message", "second command");
  catalog->Set("second_command_description", second_command_description_tree);

  base::DictionaryValue* url_country_tree = new base::DictionaryValue();
  url_country_tree->SetString("message", "de");
  catalog->Set("country", url_country_tree);

  std::vector<linked_ptr<base::DictionaryValue> > catalogs;
  catalogs.push_back(catalog);

  std::string error;
  MessageBundle* bundle = MessageBundle::Create(catalogs, &error);
  EXPECT_TRUE(bundle);
  EXPECT_TRUE(error.empty());

  return bundle;
}

TEST(ExtensionL10nUtil, LocalizeEmptyManifest) {
  base::DictionaryValue manifest;
  std::string error;
  scoped_ptr<MessageBundle> messages(CreateManifestBundle());

  EXPECT_FALSE(
      extension_l10n_util::LocalizeManifest(*messages, &manifest, &error));
  EXPECT_EQ(std::string(errors::kInvalidName), error);
}

TEST(ExtensionL10nUtil, LocalizeManifestWithoutNameMsgAndEmptyDescription) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "no __MSG");
  std::string error;
  scoped_ptr<MessageBundle> messages(CreateManifestBundle());

  EXPECT_TRUE(
      extension_l10n_util::LocalizeManifest(*messages, &manifest, &error));

  std::string result;
  ASSERT_TRUE(manifest.GetString(keys::kName, &result));
  EXPECT_EQ("no __MSG", result);

  EXPECT_FALSE(manifest.HasKey(keys::kDescription));

  EXPECT_TRUE(error.empty());
}

TEST(ExtensionL10nUtil, LocalizeManifestWithNameMsgAndEmptyDescription) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "__MSG_name__");
  std::string error;
  scoped_ptr<MessageBundle> messages(CreateManifestBundle());

  EXPECT_TRUE(
      extension_l10n_util::LocalizeManifest(*messages, &manifest, &error));

  std::string result;
  ASSERT_TRUE(manifest.GetString(keys::kName, &result));
  EXPECT_EQ("name", result);

  EXPECT_FALSE(manifest.HasKey(keys::kDescription));

  EXPECT_TRUE(error.empty());
}

TEST(ExtensionL10nUtil, LocalizeManifestWithLocalLaunchURL) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "name");
  manifest.SetString(keys::kLaunchLocalPath, "__MSG_launch_local_path__");
  std::string error;
  scoped_ptr<MessageBundle> messages(CreateManifestBundle());

  EXPECT_TRUE(
      extension_l10n_util::LocalizeManifest(*messages, &manifest, &error));

  std::string result;
  ASSERT_TRUE(manifest.GetString(keys::kLaunchLocalPath, &result));
  EXPECT_EQ("main.html", result);

  EXPECT_TRUE(error.empty());
}

TEST(ExtensionL10nUtil, LocalizeManifestWithHostedLaunchURL) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "name");
  manifest.SetString(keys::kLaunchWebURL, "__MSG_launch_web_url__");
  std::string error;
  scoped_ptr<MessageBundle> messages(CreateManifestBundle());

  EXPECT_TRUE(
      extension_l10n_util::LocalizeManifest(*messages, &manifest, &error));

  std::string result;
  ASSERT_TRUE(manifest.GetString(keys::kLaunchWebURL, &result));
  EXPECT_EQ("http://www.google.com/", result);

  EXPECT_TRUE(error.empty());
}

TEST(ExtensionL10nUtil, LocalizeManifestWithBadNameMsg) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "__MSG_name_is_bad__");
  manifest.SetString(keys::kDescription, "__MSG_description__");
  std::string error;
  scoped_ptr<MessageBundle> messages(CreateManifestBundle());

  EXPECT_FALSE(
      extension_l10n_util::LocalizeManifest(*messages, &manifest, &error));

  std::string result;
  ASSERT_TRUE(manifest.GetString(keys::kName, &result));
  EXPECT_EQ("__MSG_name_is_bad__", result);

  ASSERT_TRUE(manifest.GetString(keys::kDescription, &result));
  EXPECT_EQ("__MSG_description__", result);

  EXPECT_EQ("Variable __MSG_name_is_bad__ used but not defined.", error);
}

TEST(ExtensionL10nUtil, LocalizeManifestWithNameDescriptionDefaultTitleMsgs) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "__MSG_name__");
  manifest.SetString(keys::kDescription, "__MSG_description__");
  std::string action_title(keys::kBrowserAction);
  action_title.append(".");
  action_title.append(keys::kPageActionDefaultTitle);
  manifest.SetString(action_title, "__MSG_title__");

  std::string error;
  scoped_ptr<MessageBundle> messages(CreateManifestBundle());

  EXPECT_TRUE(
      extension_l10n_util::LocalizeManifest(*messages, &manifest, &error));

  std::string result;
  ASSERT_TRUE(manifest.GetString(keys::kName, &result));
  EXPECT_EQ("name", result);

  ASSERT_TRUE(manifest.GetString(keys::kDescription, &result));
  EXPECT_EQ("description", result);

  ASSERT_TRUE(manifest.GetString(action_title, &result));
  EXPECT_EQ("action title", result);

  EXPECT_TRUE(error.empty());
}

TEST(ExtensionL10nUtil, LocalizeManifestWithNameDescriptionOmniboxMsgs) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "__MSG_name__");
  manifest.SetString(keys::kDescription, "__MSG_description__");
  manifest.SetString(keys::kOmniboxKeyword, "__MSG_omnibox_keyword__");

  std::string error;
  scoped_ptr<MessageBundle> messages(CreateManifestBundle());

  EXPECT_TRUE(
      extension_l10n_util::LocalizeManifest(*messages, &manifest, &error));

  std::string result;
  ASSERT_TRUE(manifest.GetString(keys::kName, &result));
  EXPECT_EQ("name", result);

  ASSERT_TRUE(manifest.GetString(keys::kDescription, &result));
  EXPECT_EQ("description", result);

  ASSERT_TRUE(manifest.GetString(keys::kOmniboxKeyword, &result));
  EXPECT_EQ("omnibox keyword", result);

  EXPECT_TRUE(error.empty());
}

TEST(ExtensionL10nUtil, LocalizeManifestWithNameDescriptionFileHandlerTitle) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "__MSG_name__");
  manifest.SetString(keys::kDescription, "__MSG_description__");
  base::ListValue* handlers = new base::ListValue();
  manifest.Set(keys::kFileBrowserHandlers, handlers);
  base::DictionaryValue* handler = new base::DictionaryValue();
  handlers->Append(handler);
  handler->SetString(keys::kPageActionDefaultTitle,
                     "__MSG_file_handler_title__");

  std::string error;
  scoped_ptr<MessageBundle> messages(CreateManifestBundle());

  EXPECT_TRUE(
      extension_l10n_util::LocalizeManifest(*messages, &manifest, &error));

  std::string result;
  ASSERT_TRUE(manifest.GetString(keys::kName, &result));
  EXPECT_EQ("name", result);

  ASSERT_TRUE(manifest.GetString(keys::kDescription, &result));
  EXPECT_EQ("description", result);

  ASSERT_TRUE(handler->GetString(keys::kPageActionDefaultTitle, &result));
  EXPECT_EQ("file handler title", result);

  EXPECT_TRUE(error.empty());
}

TEST(ExtensionL10nUtil, LocalizeManifestWithNameDescriptionCommandDescription) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "__MSG_name__");
  manifest.SetString(keys::kDescription, "__MSG_description__");
  base::DictionaryValue* commands = new base::DictionaryValue();
  std::string commands_title(keys::kCommands);
  manifest.Set(commands_title, commands);

  base::DictionaryValue* first_command = new base::DictionaryValue();
  commands->Set("first_command", first_command);
  first_command->SetString(keys::kDescription,
                           "__MSG_first_command_description__");

  base::DictionaryValue* second_command = new base::DictionaryValue();
  commands->Set("second_command", second_command);
  second_command->SetString(keys::kDescription,
                            "__MSG_second_command_description__");

  std::string error;
  scoped_ptr<MessageBundle> messages(CreateManifestBundle());

  EXPECT_TRUE(
      extension_l10n_util::LocalizeManifest(*messages, &manifest, &error));

  std::string result;
  ASSERT_TRUE(manifest.GetString(keys::kName, &result));
  EXPECT_EQ("name", result);

  ASSERT_TRUE(manifest.GetString(keys::kDescription, &result));
  EXPECT_EQ("description", result);

  ASSERT_TRUE(manifest.GetString("commands.first_command.description",
                                 &result));
  EXPECT_EQ("first command", result);

  ASSERT_TRUE(manifest.GetString("commands.second_command.description",
                                 &result));
  EXPECT_EQ("second command", result);

  EXPECT_TRUE(error.empty());
}

TEST(ExtensionL10nUtil, LocalizeManifestWithShortName) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "extension name");
  manifest.SetString(keys::kShortName, "__MSG_short_name__");

  std::string error;
  scoped_ptr<MessageBundle> messages(CreateManifestBundle());

  EXPECT_TRUE(
      extension_l10n_util::LocalizeManifest(*messages, &manifest, &error));
  EXPECT_TRUE(error.empty());

  std::string result;
  ASSERT_TRUE(manifest.GetString(keys::kShortName, &result));
  EXPECT_EQ("short_name", result);
}

TEST(ExtensionL10nUtil, LocalizeManifestWithBadShortName) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "extension name");
  manifest.SetString(keys::kShortName, "__MSG_short_name_bad__");

  std::string error;
  scoped_ptr<MessageBundle> messages(CreateManifestBundle());

  EXPECT_FALSE(
      extension_l10n_util::LocalizeManifest(*messages, &manifest, &error));
  EXPECT_FALSE(error.empty());

  std::string result;
  ASSERT_TRUE(manifest.GetString(keys::kShortName, &result));
  EXPECT_EQ("__MSG_short_name_bad__", result);
}

TEST(ExtensionL10nUtil, LocalizeManifestWithSearchProviderMsgs) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kName, "__MSG_name__");
  manifest.SetString(keys::kDescription, "__MSG_description__");

  base::DictionaryValue* search_provider = new base::DictionaryValue;
  search_provider->SetString("name", "__MSG_country__");
  search_provider->SetString("keyword", "__MSG_omnibox_keyword__");
  search_provider->SetString("search_url", "http://www.foo.__MSG_country__");
  search_provider->SetString("favicon_url", "http://www.foo.__MSG_country__");
  search_provider->SetString("suggest_url", "http://www.foo.__MSG_country__");
  manifest.Set(keys::kSearchProvider, search_provider);

  std::string error;
  scoped_ptr<MessageBundle> messages(CreateManifestBundle());

  EXPECT_TRUE(
      extension_l10n_util::LocalizeManifest(*messages, &manifest, &error));

  std::string result;
  ASSERT_TRUE(manifest.GetString(keys::kName, &result));
  EXPECT_EQ("name", result);

  ASSERT_TRUE(manifest.GetString(keys::kDescription, &result));
  EXPECT_EQ("description", result);

  std::string key_prefix(keys::kSearchProvider);
  key_prefix += '.';
  ASSERT_TRUE(manifest.GetString(key_prefix + "name", &result));
  EXPECT_EQ("de", result);

  ASSERT_TRUE(manifest.GetString(key_prefix + "keyword", &result));
  EXPECT_EQ("omnibox keyword", result);

  ASSERT_TRUE(manifest.GetString(key_prefix + "search_url", &result));
  EXPECT_EQ("http://www.foo.de", result);

  ASSERT_TRUE(manifest.GetString(key_prefix + "favicon_url", &result));
  EXPECT_EQ("http://www.foo.de", result);

  ASSERT_TRUE(manifest.GetString(key_prefix + "suggest_url", &result));
  EXPECT_EQ("http://www.foo.de", result);

  EXPECT_TRUE(error.empty());
}

// Try with NULL manifest.
TEST(ExtensionL10nUtil, ShouldRelocalizeManifestWithNullManifest) {
  EXPECT_FALSE(extension_l10n_util::ShouldRelocalizeManifest(NULL));
}

// Try with default and current locales missing.
TEST(ExtensionL10nUtil, ShouldRelocalizeManifestEmptyManifest) {
  base::DictionaryValue manifest;
  EXPECT_FALSE(extension_l10n_util::ShouldRelocalizeManifest(&manifest));
}

// Try with missing current_locale.
TEST(ExtensionL10nUtil, ShouldRelocalizeManifestWithDefaultLocale) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kDefaultLocale, "en_US");
  EXPECT_TRUE(extension_l10n_util::ShouldRelocalizeManifest(&manifest));
}

// Try with missing default_locale.
TEST(ExtensionL10nUtil, ShouldRelocalizeManifestWithCurrentLocale) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kCurrentLocale,
                     extension_l10n_util::CurrentLocaleOrDefault());
  EXPECT_FALSE(extension_l10n_util::ShouldRelocalizeManifest(&manifest));
}

// Try with all data present, but with same current_locale as system locale.
TEST(ExtensionL10nUtil, ShouldRelocalizeManifestSameCurrentLocale) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kDefaultLocale, "en_US");
  manifest.SetString(keys::kCurrentLocale,
                     extension_l10n_util::CurrentLocaleOrDefault());
  EXPECT_FALSE(extension_l10n_util::ShouldRelocalizeManifest(&manifest));
}

// Try with all data present, but with different current_locale.
TEST(ExtensionL10nUtil, ShouldRelocalizeManifestDifferentCurrentLocale) {
  base::DictionaryValue manifest;
  manifest.SetString(keys::kDefaultLocale, "en_US");
  manifest.SetString(keys::kCurrentLocale, "sr");
  EXPECT_TRUE(extension_l10n_util::ShouldRelocalizeManifest(&manifest));
}

TEST(ExtensionL10nUtil, GetAllFallbackLocales) {
  std::vector<std::string> fallback_locales;
  extension_l10n_util::GetAllFallbackLocales("en_US", "all", &fallback_locales);
  ASSERT_EQ(3U, fallback_locales.size());

  CHECK_EQ("en_US", fallback_locales[0]);
  CHECK_EQ("en", fallback_locales[1]);
  CHECK_EQ("all", fallback_locales[2]);
}

}  // namespace
