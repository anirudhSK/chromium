// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/spellcheck/spellcheck_api.h"

#include "base/lazy_instance.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/spellchecker/spellcheck_factory.h"
#include "chrome/browser/spellchecker/spellcheck_service.h"
#include "chrome/common/extensions/api/spellcheck/spellcheck_handler.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {

namespace errors = manifest_errors;

namespace {

SpellcheckDictionaryInfo* GetSpellcheckDictionaryInfo(
    const Extension* extension) {
  SpellcheckDictionaryInfo *spellcheck_info =
      static_cast<SpellcheckDictionaryInfo*>(
          extension->GetManifestData(manifest_keys::kSpellcheck));
  return spellcheck_info;
}

SpellcheckService::DictionaryFormat GetDictionaryFormat(std::string format) {
  if (format == "hunspell") {
    return SpellcheckService::DICT_HUNSPELL;
  } else if (format == "text") {
    return SpellcheckService::DICT_TEXT;
  } else {
    return SpellcheckService::DICT_UNKNOWN;
  }
}

}  // namespace

SpellcheckAPI::SpellcheckAPI(content::BrowserContext* context) {
  Profile* profile = Profile::FromBrowserContext(context);
  registrar_.Add(this, chrome::NOTIFICATION_EXTENSION_LOADED,
                 content::Source<Profile>(profile));
  registrar_.Add(this, chrome::NOTIFICATION_EXTENSION_UNLOADED,
                 content::Source<Profile>(profile));
}

SpellcheckAPI::~SpellcheckAPI() {
}

static base::LazyInstance<BrowserContextKeyedAPIFactory<SpellcheckAPI> >
    g_factory = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<SpellcheckAPI>*
SpellcheckAPI::GetFactoryInstance() {
  return g_factory.Pointer();
}

void SpellcheckAPI::Observe(int type,
                            const content::NotificationSource& source,
                            const content::NotificationDetails& details) {
  Profile* profile = content::Source<Profile>(source).ptr();
  SpellcheckService* spellcheck = NULL;
  switch (type) {
    case chrome::NOTIFICATION_EXTENSION_LOADED: {
      const Extension* extension = content::Details<Extension>(details).ptr();
      SpellcheckDictionaryInfo* spellcheck_info =
          GetSpellcheckDictionaryInfo(extension);
      if (spellcheck_info) {
        // TODO(rlp): Handle load failure. =
        spellcheck = SpellcheckServiceFactory::GetForContext(profile);
        spellcheck->LoadExternalDictionary(
            spellcheck_info->language,
            spellcheck_info->locale,
            spellcheck_info->path,
            GetDictionaryFormat(spellcheck_info->format));
      }
      break;
    }
    case chrome::NOTIFICATION_EXTENSION_UNLOADED: {
      const Extension* extension =
          content::Details<UnloadedExtensionInfo>(details)->extension;
      SpellcheckDictionaryInfo* spellcheck_info =
          GetSpellcheckDictionaryInfo(extension);
      if (spellcheck_info) {
        // TODO(rlp): Handle unload failure.
        spellcheck = SpellcheckServiceFactory::GetForContext(profile);
        spellcheck->UnloadExternalDictionary(spellcheck_info->path);
      }
      break;
    }
    default:
      NOTREACHED();
  }
}

template <>
void
BrowserContextKeyedAPIFactory<SpellcheckAPI>::DeclareFactoryDependencies() {
  DependsOn(SpellcheckServiceFactory::GetInstance());
}

}  // namespace extensions
