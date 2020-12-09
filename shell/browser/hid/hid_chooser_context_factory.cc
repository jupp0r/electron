// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/hid/hid_chooser_context_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "shell/browser/hid/hid_chooser_context.h"

namespace electron {

// static
HidChooserContextFactory* HidChooserContextFactory::GetInstance() {
  static base::NoDestructor<HidChooserContextFactory> factory;
  return factory.get();
}

// static
HidChooserContext* HidChooserContextFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<HidChooserContext*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

HidChooserContextFactory::HidChooserContextFactory()
    : BrowserContextKeyedServiceFactory(
          "HidChooserContext",
          BrowserContextDependencyManager::GetInstance()) {}

HidChooserContextFactory::~HidChooserContextFactory() = default;

KeyedService* HidChooserContextFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new HidChooserContext();
}

content::BrowserContext* HidChooserContextFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context;
}

}  // namespace electron