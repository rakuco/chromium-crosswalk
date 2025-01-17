// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative/rules_registry_service.h"

#include <stddef.h>
#include <utility>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread.h"
#include "extensions/browser/api/declarative/test_rules_registry.h"
#include "extensions/browser/api/declarative_webrequest/webrequest_constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/value_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const char kExtensionId[] = "foo";

void InsertRule(scoped_refptr<extensions::RulesRegistry> registry,
                const std::string& id) {
  std::vector<linked_ptr<extensions::api::events::Rule>> add_rules;
  add_rules.push_back(make_linked_ptr(new extensions::api::events::Rule));
  add_rules[0]->id.reset(new std::string(id));
  std::string error = registry->AddRules(kExtensionId, add_rules);
  EXPECT_TRUE(error.empty());
}

void VerifyNumberOfRules(scoped_refptr<extensions::RulesRegistry> registry,
                         size_t expected_number_of_rules) {
  std::vector<linked_ptr<extensions::api::events::Rule>> get_rules;
  registry->GetAllRules(kExtensionId, &get_rules);
  EXPECT_EQ(expected_number_of_rules, get_rules.size());
}

}  // namespace

namespace extensions {

class RulesRegistryServiceTest : public testing::Test {
 public:
  RulesRegistryServiceTest()
      : ui_(content::BrowserThread::UI, &message_loop_),
        io_(content::BrowserThread::IO, &message_loop_) {}

  ~RulesRegistryServiceTest() override {}

  void TearDown() override {
    // Make sure that deletion traits of all registries are executed.
    message_loop_.RunUntilIdle();
  }

 protected:
  base::MessageLoop message_loop_;
  content::TestBrowserThread ui_;
  content::TestBrowserThread io_;
};

TEST_F(RulesRegistryServiceTest, TestConstructionAndMultiThreading) {
  RulesRegistryService registry_service(NULL);

  int key = RulesRegistryService::kDefaultRulesRegistryID;
  TestRulesRegistry* ui_registry =
      new TestRulesRegistry(content::BrowserThread::UI, "ui", key);

  TestRulesRegistry* io_registry =
      new TestRulesRegistry(content::BrowserThread::IO, "io", key);

  // Test registration.

  registry_service.RegisterRulesRegistry(make_scoped_refptr(ui_registry));
  registry_service.RegisterRulesRegistry(make_scoped_refptr(io_registry));

  EXPECT_TRUE(registry_service.GetRulesRegistry(key, "ui").get());
  EXPECT_TRUE(registry_service.GetRulesRegistry(key, "io").get());
  EXPECT_FALSE(registry_service.GetRulesRegistry(key, "foo").get());

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&InsertRule, registry_service.GetRulesRegistry(key, "ui"),
                 "ui_task"));

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&InsertRule, registry_service.GetRulesRegistry(key, "io"),
                 "io_task"));

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&VerifyNumberOfRules,
                 registry_service.GetRulesRegistry(key, "ui"), 1));

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&VerifyNumberOfRules,
                 registry_service.GetRulesRegistry(key, "io"), 1));

  message_loop_.RunUntilIdle();

  // Test extension uninstalling.
  std::unique_ptr<base::DictionaryValue> manifest =
      DictionaryBuilder()
          .Set("name", "Extension")
          .Set("version", "1.0")
          .Set("manifest_version", 2)
          .Build();
  scoped_refptr<Extension> extension = ExtensionBuilder()
                                           .SetManifest(std::move(manifest))
                                           .SetID(kExtensionId)
                                           .Build();
  registry_service.SimulateExtensionUninstalled(extension.get());

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&VerifyNumberOfRules,
                 registry_service.GetRulesRegistry(key, "ui"), 0));

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&VerifyNumberOfRules,
                 registry_service.GetRulesRegistry(key, "io"), 0));

  message_loop_.RunUntilIdle();
}

}  // namespace extensions
