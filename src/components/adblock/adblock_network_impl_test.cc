/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "components/adblock/adblock_network_impl.h"

#include "base/run_loop.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

using adblock::AdblockNetworkImpl;

namespace {

const char kTestURL[] = "http://faketesturl.com";
const char kPassedResponse[] = "PASSED";

}  // namespace

class AbpNetworkTest : public testing::Test {
 public:
  AbpNetworkTest() {
    abp_task_runner_ = base::ThreadPool::CreateSingleThreadTaskRunner(
        {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
        base::SingleThreadTaskRunnerThreadMode::DEDICATED);
    ui_task_runner_ = base::ThreadTaskRunnerHandle::Get();
  }
  ~AbpNetworkTest() override {}

  AbpNetworkTest(const AbpNetworkTest&) = delete;
  AbpNetworkTest& operator=(const AbpNetworkTest&) = delete;

  network::TestURLLoaderFactory* net_factory() const {
    return const_cast<network::TestURLLoaderFactory*>(
        &test_url_loader_factory_);
  }

  void SetUp() override {
    base::RunLoop loop;
    abp_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&AbpNetworkTest::CreateNetworkOnABPRunner,
                                  base::Unretained(this), &loop));
    loop.Run();
  }

  void TearDown() override {
    abp_task_runner_->DeleteSoon(FROM_HERE, std::move(network_));
  }

  adblock::AdblockNetworkImpl* network() const { return network_.get(); }

  scoped_refptr<base::SingleThreadTaskRunner> abp_task_runner() const {
    return abp_task_runner_;
  }

  void DoTest(const char* method,
              const std::string& expected_response,
              net::HttpStatusCode expected_status) {
    base::RunLoop loop;
    ASSERT_NE(nullptr, network());
    net_factory()->AddResponse(kTestURL, expected_response, expected_status);
    int hit_cnt = 0;
    net_factory()->SetInterceptor(base::BindLambdaForTesting(
        [&hit_cnt](const network::ResourceRequest& request) { ++hit_cnt; }));
    abp_task_runner()->PostTask(
        FROM_HERE,
        base::BindLambdaForTesting([this, &loop, method, &expected_response,
                                    expected_status]() {
          auto method_ptr = (method == net::HttpRequestHeaders::kGetMethod)
                                ? &AdblockNetworkImpl::GET
                                : &AdblockNetworkImpl::HEAD;

          (network()->*method_ptr)(
              kTestURL, {},
              [net = network(), &loop, &expected_response,
               expected_status](const AdblockPlus::ServerResponse& response) {
                EXPECT_EQ(response.responseText, expected_response);
                EXPECT_EQ(expected_status, response.responseStatus);
                EXPECT_EQ(AdblockPlus::IWebRequest::NS_OK, response.status);
                loop.Quit();
              });
        }));
    loop.Run();
    EXPECT_EQ(1, hit_cnt);
  }

 private:
  void CreateNetworkOnABPRunner(base::RunLoop* loop) {
    network_ = std::make_unique<adblock::AdblockNetworkImpl>(
        ui_task_runner_, abp_task_runner_, net_factory());
    loop->Quit();
  }

  base::test::TaskEnvironment task_env_;
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> abp_task_runner_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  std::unique_ptr<AdblockNetworkImpl> network_;
};

TEST_F(AbpNetworkTest, LoadOKGET) {
  DoTest(net::HttpRequestHeaders::kGetMethod, kPassedResponse, net::HTTP_OK);
}

TEST_F(AbpNetworkTest, LoadFailureGET) {
  DoTest(net::HttpRequestHeaders::kGetMethod, "", net::HTTP_BAD_REQUEST);
}

TEST_F(AbpNetworkTest, LoadOKHEAD) {
  DoTest(net::HttpRequestHeaders::kHeadMethod, kPassedResponse, net::HTTP_OK);
}

TEST_F(AbpNetworkTest, LoadFailureHEAD) {
  DoTest(net::HttpRequestHeaders::kHeadMethod, "", net::HTTP_BAD_REQUEST);
}