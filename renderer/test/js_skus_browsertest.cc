/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/profiles/brave_renderer_updater.h"
#include "brave/browser/profiles/brave_renderer_updater_factory.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/skus/common/features.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace skus {

class JsSkusBrowserTest : public InProcessBrowserTest {
 public:
  JsSkusBrowserTest() : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.ServeFilesFromDirectory(test_data_dir);
  }

  ~JsSkusBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    // Map all hosts to localhost.
    host_resolver()->AddRule("*", "127.0.0.1");
    EXPECT_TRUE(https_server_.Start());
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* main_frame() {
    return web_contents()->GetMainFrame();
  }

  void SetSkusAllowedOriginForTesting(const GURL& url) {
    auto* renderer_updater =
        BraveRendererUpdaterFactory::GetForProfile(browser()->profile());
    renderer_updater->SetSkusAllowedOriginForTesting(
        url.GetWithEmptyPath().spec());
    renderer_updater->UpdateAllRenderers();
    base::RunLoop().RunUntilIdle();
  }

  void ReloadAndWaitForLoadStop() {
    chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
    ASSERT_TRUE(content::WaitForLoadStop(web_contents()));
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(JsSkusBrowserTest, AttachSkus) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("other.software", "/simple.html")));
  std::string command =
      "window.chrome.braveSkus !== undefined && "
      "window.chrome.braveSkus.refresh_order !== undefined";
  {
    // No api attached.
    auto result = content::EvalJs(main_frame(), command);
    EXPECT_EQ(result.error, "");
    ASSERT_FALSE(result.ExtractBool());
  }
  auto allowed_origin = https_server_.GetURL("a.com", "/simple.html");
  SetSkusAllowedOriginForTesting(allowed_origin);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), allowed_origin));
  {
    // API attached
    auto result = content::EvalJs(main_frame(), command);
    EXPECT_EQ(result.error, "");
    ASSERT_TRUE(result.ExtractBool());
  }

  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
  ReloadAndWaitForLoadStop();
  {
    // api attached after reload
    auto result = content::EvalJs(main_frame(), command);
    EXPECT_EQ(result.error, "");
    ASSERT_TRUE(result.ExtractBool());
  }
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
  // overwrite failed
  std::string overwrite =
      "window.chrome.braveSkus = ['test'];window.chrome.braveSkus[0]";
  EXPECT_EQ(content::EvalJs(main_frame(), overwrite).error, "");
  ASSERT_TRUE(content::EvalJs(main_frame(), command).ExtractBool());
  ReloadAndWaitForLoadStop();
}

}  // namespace skus
