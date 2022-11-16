// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/cookies_tree_model.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/request_handler_util.h"
#include "third_party/blink/public/common/features.h"

namespace {

// Calls the accessStorage javascript function and awaits its completion for
// each frame in the active web contents for |browser|.
void EnsurePageAccessedStorage(content::WebContents* web_contents) {
  web_contents->GetPrimaryMainFrame()->ForEachRenderFrameHost(
      base::BindRepeating([](content::RenderFrameHost* frame) {
        EXPECT_TRUE(
            content::EvalJs(frame,
                            "(async () => { return await accessStorage();})()")
                .value.GetBool());
      }));
}

std::vector<CookieTreeNode*> GetAllChildNodes(CookieTreeNode* node) {
  std::vector<CookieTreeNode*> nodes;
  for (const auto& child : node->children()) {
    auto child_nodes = GetAllChildNodes(child.get());
    nodes.insert(nodes.end(), child_nodes.begin(), child_nodes.end());
  }
  nodes.push_back(node);
  return nodes;
}

std::map<CookieTreeNode::DetailedInfo::NodeType, int> GetNodeTypeCounts(
    CookiesTreeModel* model) {
  auto nodes = GetAllChildNodes(model->GetRoot());

  std::map<CookieTreeNode::DetailedInfo::NodeType, int> node_counts_map;
  for (auto* node : nodes)
    node_counts_map[node->GetDetailedInfo().node_type]++;

  return node_counts_map;
}

}  // namespace

class CookiesTreeObserver : public CookiesTreeModel::Observer {
 public:
  void AwaitTreeModelEndBatch() {
    run_loop = std::make_unique<base::RunLoop>();
    run_loop->Run();
  }

  void TreeModelEndBatch(CookiesTreeModel* model) override { run_loop->Quit(); }

  void TreeNodesAdded(ui::TreeModel* model,
                      ui::TreeModelNode* parent,
                      size_t start,
                      size_t count) override {}
  void TreeNodesRemoved(ui::TreeModel* model,
                        ui::TreeModelNode* parent,
                        size_t start,
                        size_t count) override {}
  void TreeNodeChanged(ui::TreeModel* model, ui::TreeModelNode* node) override {
  }

 private:
  std::unique_ptr<base::RunLoop> run_loop;
};

// TODO(crbug.com/1271155): This test copies logic from the Access Context Audit
// Service test. At least this test, and likely the ACA service & test, can be
// removed when the tree model is deprecated.
class CookiesTreeModelBrowserTest : public InProcessBrowserTest {
 public:
  void SetUp() override {
    InitFeatures();
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    test_server_.ServeFilesFromSourceDirectory(
        base::FilePath(FILE_PATH_LITERAL("content/test/data")));
    test_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    ASSERT_TRUE(test_server_.Start());
  }

  void AccessStorage() {
    ASSERT_TRUE(content::NavigateToURL(
        chrome_test_utils::GetActiveWebContents(this), storage_accessor_url()));
    base::RunLoop().RunUntilIdle();
    EnsurePageAccessedStorage(chrome_test_utils::GetActiveWebContents(this));
  }

  GURL storage_accessor_url() {
    auto host_port_pair =
        net::HostPortPair::FromURL(test_server_.GetURL("a.test", "/"));
    base::StringPairs replacement_text = {
        {"REPLACE_WITH_HOST_AND_PORT", host_port_pair.ToString()}};
    auto replaced_path = net::test_server::GetFilePathWithReplacements(
        "/browsing_data/storage_accessor.html", replacement_text);
    return test_server_.GetURL("a.test", replaced_path);
  }

  virtual void InitFeatures() {
    feature_list()->InitAndDisableFeature(
        blink::features::kThirdPartyStoragePartitioning);
  }

  base::test::ScopedFeatureList* feature_list() { return &feature_list_; }

 protected:
  net::EmbeddedTestServer test_server_{net::EmbeddedTestServer::TYPE_HTTPS};
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(CookiesTreeModelBrowserTest, NoQuotaStorage) {
  AccessStorage();

  auto tree_model = CookiesTreeModel::CreateForProfileDeprecated(
      chrome_test_utils::GetProfile(this));
  CookiesTreeObserver observer;
  tree_model->AddCookiesTreeObserver(&observer);
  observer.AwaitTreeModelEndBatch();

  // Quota storage has been accessed, but should not be present in the tree.
  EXPECT_EQ(17, tree_model->GetRoot()->GetTotalNodeCount());
  auto node_counts = GetNodeTypeCounts(tree_model.get());
  EXPECT_EQ(16u, node_counts.size());
  EXPECT_EQ(0, node_counts[CookieTreeNode::DetailedInfo::TYPE_QUOTA]);

  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_ROOT]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_HOST]);
  EXPECT_EQ(2, node_counts[CookieTreeNode::DetailedInfo::TYPE_COOKIE]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_COOKIES]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_DATABASE]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_DATABASES]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_LOCAL_STORAGE]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_LOCAL_STORAGES]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_INDEXED_DB]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_INDEXED_DBS]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_FILE_SYSTEM]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_FILE_SYSTEMS]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_SERVICE_WORKER]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_SERVICE_WORKERS]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_CACHE_STORAGE]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_CACHE_STORAGES]);
}

class CookiesTreeModelBrowserTestQuotaOnly
    : public CookiesTreeModelBrowserTest {
 public:
  void InitFeatures() override {
    feature_list()->InitAndEnableFeature(
        blink::features::kThirdPartyStoragePartitioning);
  }
};

IN_PROC_BROWSER_TEST_F(CookiesTreeModelBrowserTestQuotaOnly, QuotaStorageOnly) {
  AccessStorage();

  auto tree_model = CookiesTreeModel::CreateForProfileDeprecated(
      chrome_test_utils::GetProfile(this));
  CookiesTreeObserver observer;
  tree_model->AddCookiesTreeObserver(&observer);
  observer.AwaitTreeModelEndBatch();

  // Quota storage has been accessed, only quota nodes should be present for
  // quota managed storage types.
  EXPECT_EQ(8, tree_model->GetRoot()->GetTotalNodeCount());

  auto node_counts = GetNodeTypeCounts(tree_model.get());
  EXPECT_EQ(7u, node_counts.size());
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_ROOT]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_HOST]);
  EXPECT_EQ(2, node_counts[CookieTreeNode::DetailedInfo::TYPE_COOKIE]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_COOKIES]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_LOCAL_STORAGE]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_LOCAL_STORAGES]);
  EXPECT_EQ(1, node_counts[CookieTreeNode::DetailedInfo::TYPE_QUOTA]);
}
