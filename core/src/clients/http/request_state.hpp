#pragma once

#include <array>
#include <cstdlib>
#include <optional>
#include <string>
#include <system_error>

#include <userver/clients/http/destination_statistics.hpp>
#include <userver/clients/http/enforce_task_deadline_config.hpp>
#include <userver/clients/http/error.hpp>
#include <userver/clients/http/form.hpp>
#include <userver/clients/http/response_future.hpp>
#include <userver/clients/http/statistics.hpp>
#include <userver/crypto/certificate.hpp>
#include <userver/crypto/private_key.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/blocking_future.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/url.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>

#include <clients/http/easy_wrapper.hpp>
#include <clients/http/testsuite.hpp>
#include <crypto/helpers.hpp>
#include <engine/ev/watcher/timer_watcher.hpp>

namespace clients::http {

class RequestState : public std::enable_shared_from_this<RequestState> {
 public:
  RequestState(std::shared_ptr<impl::EasyWrapper>&&,
               std::shared_ptr<RequestStats>&& req_stats,
               const std::shared_ptr<DestinationStatistics>& dest_stats);
  ~RequestState();

  /// Perform async http request
  engine::impl::BlockingFuture<std::shared_ptr<Response>> async_perform();

  /// set redirect flags
  void follow_redirects(bool follow);
  /// set verify flags
  void verify(bool verify);
  /// set file holding one or more certificates to verify the peer with
  void ca_info(const std::string& file_path);
  /// set dir with CA certificates
  void ca_file(const std::string& dir_path);
  /// set CRL-file
  void crl_file(const std::string& file_path);
  /// set private key and certificate from memory
  void client_key_cert(crypto::PrivateKey pkey, crypto::Certificate cert);
  /// Set HTTP version
  void http_version(curl::easy::http_version_t version);
  /// set timeout value
  void set_timeout(long timeout_ms);
  /// set number of retries
  void retry(short retries, bool on_fails);
  /// set unix socket as transport instead of TCP
  void unix_socket_path(const std::string& path);

  /// get timeout value
  long timeout() const { return timeout_ms_; }
  /// get retries count
  short retries() const { return retry_.retries; }

  /// cancel request
  void Cancel();

  void SetDestinationMetricNameAuto(std::string destination);

  void SetDestinationMetricName(const std::string& destination);

  void SetTestsuiteConfig(const std::shared_ptr<const TestsuiteConfig>& config);

  void DisableReplyDecoding();

  void EnableAddClientTimeoutHeader();
  void DisableAddClientTimeoutHeader();
  void SetEnforceTaskDeadline(EnforceTaskDeadlineConfig enforce_task_deadline);

  std::shared_ptr<impl::EasyWrapper> easy_wrapper() { return easy_; }

  curl::easy& easy() { return easy_->Easy(); }
  const curl::easy& easy() const { return easy_->Easy(); }
  std::shared_ptr<Response> response() const { return response_; }
  std::shared_ptr<Response> response_move() { return std::move(response_); }

 private:
  /// final callback that calls user callback and set value in promise
  static void on_completed(std::shared_ptr<RequestState>, std::error_code err);
  /// retry callback
  static void on_retry(std::shared_ptr<RequestState>, std::error_code err);
  /// header function curl callback
  static size_t on_header(void* ptr, size_t size, size_t nmemb, void* userdata);

  /// certifiacte function curl callback
  static curl::native::CURLcode on_certificate_request(void* curl, void* sslctx,
                                                       void* userdata) noexcept;

  /// parse one header
  void parse_header(char* ptr, size_t size);
  /// simply run perform_request if there is now errors from timer
  void on_retry_timer(std::error_code err);
  /// run curl async_request
  void perform_request(curl::easy::handler_type handler);

  uint64_t GetClientTimeoutMs() const;
  void UpdateClientTimeoutHeader(uint64_t client_timeout_ms);

  void AccountResponse(std::error_code err);

  void ApplyTestsuiteConfig();

 private:
  /// curl handler wrapper
  std::shared_ptr<impl::EasyWrapper> easy_;
  std::shared_ptr<RequestStats> stats_;
  std::shared_ptr<RequestStats> dest_req_stats_;

  std::shared_ptr<DestinationStatistics> dest_stats_;
  std::string destination_metric_name_;

  std::shared_ptr<const TestsuiteConfig> testsuite_config_;

  crypto::PrivateKey pkey_;
  crypto::Certificate cert_;

  /// response
  std::shared_ptr<Response> response_;
  engine::impl::BlockingPromise<std::shared_ptr<Response>> promise_;
  /// timeout value
  long timeout_ms_;

  bool add_client_timeout_header_{true};
  EnforceTaskDeadlineConfig enforce_task_deadline_{};
  /// deadline from current task
  engine::Deadline deadline_;
  /// struct for reties
  struct {
    /// maximum number of retries
    short retries{1};
    /// current retry
    short current{1};
    /// flag for treating network errors as reason for retry
    bool on_fails{false};
    /// pointer to timer
    std::unique_ptr<engine::ev::TimerWatcher> timer;
  } retry_;

  std::optional<tracing::Span> span_;
  bool disable_reply_decoding_;

  std::atomic<bool> is_cancelled_;
  std::array<char, CURL_ERROR_SIZE> errorbuffer_;
};

}  // namespace clients::http
