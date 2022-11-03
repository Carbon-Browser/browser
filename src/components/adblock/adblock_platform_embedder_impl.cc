#include "components/adblock/adblock_platform_embedder_impl.h"

#include "AdblockPlus/IFilterEngine.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "components/adblock/adblock_constants.h"
#include "components/adblock/adblock_controller.h"
#include "components/adblock/adblock_filesystem_impl.h"
#include "components/adblock/adblock_logging_impl.h"
#include "components/adblock/adblock_network_impl.h"
#include "components/adblock/adblock_platform_embedder.h"
#include "components/adblock/adblock_resource_reader_impl.h"
#include "components/adblock/adblock_switches.h"
#include "components/adblock/adblock_timer_impl.h"
#include "components/adblock/allowed_connection_type.h"
#include "components/prefs/pref_service.h"
#include "gin/public/isolate_holder.h"
#include "gin/v8_initializer.h"
#include "net/base/network_change_notifier.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

#include "third_party/libadblockplus/src/include/AdblockPlus.h"
#include "third_party/libadblockplus/src/include/AdblockPlus/FilterEngineFactory.h"
#include "third_party/libadblockplus/src/include/AdblockPlus/Platform.h"

namespace {

class IsolateHolderV8Provider : public AdblockPlus::IV8IsolateProvider {
 public:
  explicit IsolateHolderV8Provider(std::unique_ptr<gin::IsolateHolder> holder)
      : isolate_holder_(std::move(holder)) {}

  v8::Isolate* Get() override { return isolate_holder_->isolate(); }

  ~IsolateHolderV8Provider() override {
    VLOG(3) << "[ABP] ~IsolateHolderV8Provider() for " << this;
  }

  IsolateHolderV8Provider(const IsolateHolderV8Provider&) = delete;
  IsolateHolderV8Provider& operator=(const IsolateHolderV8Provider&) = delete;

 private:
  std::unique_ptr<gin::IsolateHolder> isolate_holder_;
};

void IsConnectionTypeAllowed(const std::string* allowed_connection_type_str,
                             const std::function<void(bool)>& reply) {
  bool allowed = true;
  auto allowed_connection_type = adblock::AllowedConnectionTypeFromString(
      allowed_connection_type_str ? *allowed_connection_type_str : "");
  if (allowed_connection_type &&
      *allowed_connection_type != adblock::AllowedConnectionType::kAny) {
    auto type = net::NetworkChangeNotifier::GetConnectionType();
    DLOG(INFO) << "[ABP] Connection type returned from the net notifier: "
               << type;
    allowed =
        *allowed_connection_type == adblock::AllowedConnectionType::kWiFi &&
        (type == net::NetworkChangeNotifier::ConnectionType::CONNECTION_WIFI ||
         // VPNs make Chromium not know the connection type. Allow loading in
         // that case to ensure filter lists get downloaded.
         type ==
             net::NetworkChangeNotifier::ConnectionType::CONNECTION_UNKNOWN);
  }

  DLOG(INFO) << "[ABP] Connection of type "
             << (allowed_connection_type_str ? *allowed_connection_type_str
                                             : "unknown")
             << " is " << (allowed ? "allowed" : "not allowed");
  reply(allowed);
}

struct ComposeFilterSuggestionsReply {
  std::unique_ptr<AdblockPlus::IElement> element;
  std::vector<std::string> filters;
};

ComposeFilterSuggestionsReply ComposeFilterSuggestionsHelper(
    AdblockPlus::IFilterEngine* filter_engine,
    std::unique_ptr<AdblockPlus::IElement> element) {
  DCHECK(filter_engine);

  auto* bare_element = element.get();
  return ComposeFilterSuggestionsReply{
      std::move(element),
      filter_engine->ComposeFilterSuggestions(bare_element)};
}

void ComposeFilterSuggestionsReplyHelper(
    base::OnceCallback<void(const std::vector<std::string>&)> callback,
    const ComposeFilterSuggestionsReply& reply) {
  std::move(callback).Run(reply.filters);
}

}  // namespace

namespace adblock {

AdblockPlatformEmbedderImpl::AdblockPlatformEmbedderImpl(
    scoped_refptr<utils::TaskRunnerWrapper> abp_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
    scoped_refptr<network::SharedURLLoaderFactory> loader_factory,
    const std::string& locale,
    base::FilePath storage_dir,
    PrefService* pref_service)
    : AdblockPlatformEmbedder(abp_runner),
      abp_runner_(abp_runner),
      ui_runner_(ui_runner),
      loader_factory_(loader_factory),
      locale_(locale),
      storage_dir_(std::move(storage_dir)),
      pref_service_(pref_service) {}

AdblockPlatformEmbedderImpl::~AdblockPlatformEmbedderImpl() {
  DCHECK(abp_runner_->RunsTasksInCurrentSequence());
  if (filter_engine_)
    filter_engine_->RemoveEventObserver(this);
  abp_runner_->DisallowExecution();
}

void AdblockPlatformEmbedderImpl::ShutdownOnUIThread() {
  DCHECK(ui_runner_->RunsTasksInCurrentSequence());
  pending_jobs_.clear();
  loader_factory_.reset();
}

void AdblockPlatformEmbedderImpl::OnFilterEngineCreated(
    const AdblockPlus::IFilterEngine& engine) {
  auto init_time = base::Time::Now() - filter_engine_start_time_;
  DCHECK(abp_runner_->RunsTasksInCurrentSequence());
  LOG(INFO) << "[ABP] Filter Engine initialized after "
            << init_time.InMilliseconds() << " ms";
  SetFilterEngine(&(platform_->GetFilterEngine()));

  filter_engine_->AddEventObserver(this);

  auto recommended = engine.FetchAvailableSubscriptions();

  std::vector<Subscription> recommended_converted;
  for (auto& subscription : recommended) {
    auto title = subscription.GetTitle();
    auto languages = subscription.GetLanguages();
    auto url = subscription.GetUrl();

    recommended_converted.emplace_back(
        Subscription{GURL(url), title, languages});
  }

  auto subscriptions = engine.GetListedSubscriptions();
  std::vector<GURL> listed_converted;
  for (auto subscription : subscriptions) {
    listed_converted.emplace_back(GURL(subscription.GetUrl()));
  }

  GURL acceptable_ads(engine.GetAAUrl());

  ui_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&AdblockPlatformEmbedderImpl::NotifyEngineCreatedOnUI,
                     this, recommended_converted, listed_converted,
                     acceptable_ads));
}

void AdblockPlatformEmbedderImpl::FinishSetUpOnBG(
    const FilterEngineStartConfig& config) {
  VLOG(3) << "[ABP] FinishSetUpOnBG() started";

  auto provider = std::make_unique<IsolateHolderV8Provider>(
      std::make_unique<gin::IsolateHolder>(
          abp_runner_, gin::IsolateHolder::AccessMode::kUseLocker,
          gin::IsolateHolder::IsolateType::kUtility));

  AdblockPlus::PlatformFactory::CreationParameters platform_params{
      std::make_unique<adblock::AdblockLoggingImpl>(),
      std::make_unique<adblock::AdblockTimerImpl>(abp_runner_),
      std::make_unique<adblock::AdblockNetworkImpl>(ui_runner_, abp_runner_,
                                                    loader_factory_.get()),
      std::make_unique<adblock::AdblockFileSystemImpl>(abp_runner_,
                                                       storage_dir_),
      std::make_unique<adblock::AdblockResourceReaderImpl>(abp_runner_),
      "",  // base path
  };
  platform_ =
      AdblockPlus::PlatformFactory::CreatePlatform(std::move(platform_params));
  AdblockPlus::AppInfo app_info;
  auto internal_info = adblock::utils::GetAppInfo();
  app_info.application = internal_info.name;
  app_info.applicationVersion = internal_info.version;
  app_info.name = "libadblockplus";
  app_info.version = "1.0";

  app_info.locale = locale_;
  platform_->SetUp(app_info, std::move(provider));
  AdblockPlus::FilterEngineFactory::CreationParameters engine_params;
  engine_params.isSubscriptionDownloadAllowedCallback =
      &IsConnectionTypeAllowed;
  engine_params.preconfiguredPrefs.stringPrefs.emplace(std::make_pair(
      AdblockPlus::FilterEngineFactory::StringPrefName::AllowedConnectionType,
      AllowedConnectionTypeToString(config.allowed_connection_type)));
  engine_params.preconfiguredPrefs.booleanPrefs.emplace(std::make_pair(
      AdblockPlus::FilterEngineFactory::BooleanPrefName::AcceptableAdsEnabled,
      config.acceptable_ads_enabled));
  platform_->CreateFilterEngineAsync(
      engine_params, [this](const AdblockPlus::IFilterEngine& engine) {
        OnFilterEngineCreated(engine);
      });
}

void AdblockPlatformEmbedderImpl::SetUp(const FilterEngineStartConfig& config) {
  // v8 init
  VLOG(3) << "[ABP] Creating isolate holder ...";

#ifdef V8_USE_EXTERNAL_STARTUP_DATA
  VLOG(3) << "[ABP] Loading v8 snapshot & natives ...";
  gin::V8Initializer::LoadV8Snapshot();
  VLOG(3) << "[ABP] Loaded v8 snapshot & natives";
#endif

  VLOG(3) << "[ABP] Initialize isolate holder";
  gin::IsolateHolder::Initialize(gin::IsolateHolder::kStrictMode,
                                 gin::ArrayBufferAllocator::SharedInstance());

  AdblockFileSystemImpl::AttemptMigrationIfRequired(
      pref_service_, storage_dir_,
      base::BindOnce(&AdblockPlatformEmbedderImpl::PostFinishSetUpTask, this,
                     config));
}

void AdblockPlatformEmbedderImpl::PostFinishSetUpTask(
    const FilterEngineStartConfig& config) {
  abp_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AdblockPlatformEmbedderImpl::FinishSetUpOnBG,
                                this, config));
}

void AdblockPlatformEmbedderImpl::AddObserver(
    AdblockPlatformEmbedder::Observer* observer) {
  observers_.AddObserver(observer);
}

void AdblockPlatformEmbedderImpl::RemoveObserver(
    AdblockPlatformEmbedder::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AdblockPlatformEmbedderImpl::CreateFilterEngine(
    AdblockController* controller) {
  filter_engine_start_time_ = base::Time::Now();
  FilterEngineStartConfig config;
  config.allowed_connection_type = controller->GetUpdateConsent();
  config.acceptable_ads_enabled = controller->IsAcceptableAdsEnabled();
  SetUp(config);
}

AdblockPlus::IFilterEngine* AdblockPlatformEmbedderImpl::GetFilterEngine() {
  base::AutoLock auto_lock(filter_engine_lock_);
  return filter_engine_;
}

AdblockPlus::Platform* AdblockPlatformEmbedderImpl::GetPlatform() const {
  return platform_.get();
}

scoped_refptr<base::SingleThreadTaskRunner>
AdblockPlatformEmbedderImpl::GetAbpTaskRunner() {
  return abp_runner_;
}

void AdblockPlatformEmbedderImpl::NotifyEngineCreatedOnUI(
    const std::vector<Subscription>& recommended,
    const std::vector<GURL>& listed,
    const GURL& acceptable_ads) {
  for (auto& observer : observers_) {
    observer.OnFilterEngineCreated(recommended, listed, acceptable_ads);
  }

  for (auto& pending_job : pending_jobs_)
    std::move(pending_job).Run();
  pending_jobs_.clear();
}

void AdblockPlatformEmbedderImpl::NotifySubscriptionDownloadedOnUI(
    const AdblockPlus::Subscription& subscription) {
  GURL subscription_url{subscription.GetUrl()};
  for (auto& observer : observers_) {
    observer.OnSubscriptionUpdated(subscription_url);
  }
}

void AdblockPlatformEmbedderImpl::OnSubscriptionEvent(
    AdblockPlus::IFilterEngine::SubscriptionEvent event,
    const AdblockPlus::Subscription& subscription) {
  if (event ==
      AdblockPlus::IFilterEngine::SubscriptionEvent::SUBSCRIPTION_UPDATED) {
    ui_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &AdblockPlatformEmbedderImpl::NotifySubscriptionDownloadedOnUI,
            this, subscription));
  }
}

void AdblockPlatformEmbedderImpl::RunAfterFilterEngineCreated(
    base::OnceClosure closure) {
  DCHECK_EQ(GetFilterEngine(), nullptr);
  pending_jobs_.push_back(std::move(closure));
}

void AdblockPlatformEmbedderImpl::ComposeFilterSuggestions(
    std::unique_ptr<AdblockPlus::IElement> element,
    base::OnceCallback<void(const std::vector<std::string>&)> callback) {
  if (!GetFilterEngine()) {
    VLOG(1) << "[ABP] ComposeFilterSuggestions no engine yet";
    pending_jobs_.emplace_back(
        base::BindOnce(&AdblockPlatformEmbedderImpl::ComposeFilterSuggestions,
                       this, std::move(element), std::move(callback)));
    return;
  }

  GetAbpTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ComposeFilterSuggestionsHelper, GetFilterEngine(),
                     std::move(element)),
      base::BindOnce(&ComposeFilterSuggestionsReplyHelper,
                     std::move(callback)));
}

void AdblockPlatformEmbedderImpl::SetFilterEngine(
    AdblockPlus::IFilterEngine* engine) {
  base::AutoLock auto_lock(filter_engine_lock_);
  filter_engine_ = engine;
}

}  // namespace adblock
