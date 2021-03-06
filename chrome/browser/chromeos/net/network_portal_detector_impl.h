// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_NET_NETWORK_PORTAL_DETECTOR_IMPL_H_
#define CHROME_BROWSER_CHROMEOS_NET_NETWORK_PORTAL_DETECTOR_IMPL_H_

#include <string>

#include "base/basictypes.h"
#include "base/cancelable_callback.h"
#include "base/compiler_specific.h"
#include "base/containers/hash_tables.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/threading/non_thread_safe.h"
#include "base/time/time.h"
#include "chrome/browser/captive_portal/captive_portal_detector.h"
#include "chrome/browser/chromeos/net/network_portal_detector.h"
#include "chrome/browser/chromeos/net/network_portal_detector_strategy.h"
#include "chrome/browser/chromeos/net/network_portal_notification_controller.h"
#include "chromeos/network/network_state_handler_observer.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "net/url_request/url_fetcher.h"
#include "url/gurl.h"

namespace net {
class URLRequestContextGetter;
}

namespace chromeos {

class NetworkState;

// This class handles all notifications about network changes from
// NetworkStateHandler and delegates portal detection for the default
// network to CaptivePortalService.
class NetworkPortalDetectorImpl
    : public NetworkPortalDetector,
      public base::NonThreadSafe,
      public chromeos::NetworkStateHandlerObserver,
      public content::NotificationObserver,
      public PortalDetectorStrategy::Delegate {
 public:
  static const char kDetectionResultHistogram[];
  static const char kDetectionDurationHistogram[];
  static const char kShillOnlineHistogram[];
  static const char kShillPortalHistogram[];
  static const char kShillOfflineHistogram[];

  explicit NetworkPortalDetectorImpl(
      const scoped_refptr<net::URLRequestContextGetter>& request_context);
  virtual ~NetworkPortalDetectorImpl();

  // NetworkPortalDetector implementation:
  virtual void AddObserver(Observer* observer) OVERRIDE;
  virtual void AddAndFireObserver(Observer* observer) OVERRIDE;
  virtual void RemoveObserver(Observer* observer) OVERRIDE;
  virtual CaptivePortalState GetCaptivePortalState(
      const std::string& service_path) OVERRIDE;
  virtual bool IsEnabled() OVERRIDE;
  virtual void Enable(bool start_detection) OVERRIDE;
  virtual bool StartDetectionIfIdle() OVERRIDE;
  virtual void EnableErrorScreenStrategy() OVERRIDE;
  virtual void DisableErrorScreenStrategy() OVERRIDE;

  // NetworkStateHandlerObserver implementation:
  virtual void DefaultNetworkChanged(const NetworkState* network) OVERRIDE;

  // PortalDetectorStrategy::Delegate implementation:
  virtual int AttemptCount() OVERRIDE;
  virtual base::TimeTicks AttemptStartTime() OVERRIDE;
  virtual base::TimeTicks GetCurrentTimeTicks() OVERRIDE;

 private:
  friend class NetworkPortalDetectorImplTest;

  typedef std::string NetworkId;
  typedef base::hash_map<NetworkId, CaptivePortalState> CaptivePortalStateMap;

  enum State {
    // No portal check is running.
    STATE_IDLE = 0,
    // Waiting for portal check.
    STATE_PORTAL_CHECK_PENDING,
    // Portal check is in progress.
    STATE_CHECKING_FOR_PORTAL,
  };

  // Starts detection process.
  void StartDetection();

  // Stops whole detection process.
  void StopDetection();

  // Internal predicate which describes set of states from which
  // DetectCaptivePortal() can be called.
  bool CanPerformAttempt() const;

  // Initiates Captive Portal detection attempt after |delay|.
  // You should check CanPerformAttempt() before calling this method.
  void ScheduleAttempt(const base::TimeDelta& delay);

  // Starts detection attempt.
  void StartAttempt();

  // Called when portal check is timed out. Cancels portal check and calls
  // OnPortalDetectionCompleted() with RESULT_NO_RESPONSE as a result.
  void OnAttemptTimeout();

  // Called by CaptivePortalDetector when detection attempt completes.
  void OnAttemptCompleted(
      const captive_portal::CaptivePortalDetector::Results& results);

  // content::NotificationObserver implementation:
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // Stores captive portal state for a |network| and notifies observers.
  void OnDetectionCompleted(const NetworkState* network,
                            const CaptivePortalState& results);

  // Notifies observers that portal detection is completed for a |network|.
  void NotifyDetectionCompleted(const NetworkState* network,
                                const CaptivePortalState& state);

  State state() const { return state_; }

  bool is_idle() const {
    return state_ == STATE_IDLE;
  }
  bool is_portal_check_pending() const {
    return state_ == STATE_PORTAL_CHECK_PENDING;
  }
  bool is_checking_for_portal() const {
    return state_ == STATE_CHECKING_FOR_PORTAL;
  }

  int attempt_count_for_testing() {
    return attempt_count_;
  }

  void set_attempt_count_for_testing(int attempt_count) {
    attempt_count_ = attempt_count;
  }

  // Returns delay before next portal check. Used by unit tests.
  const base::TimeDelta& next_attempt_delay_for_testing() const {
    return next_attempt_delay_;
  }

  // Returns true if attempt timeout callback isn't fired or
  // cancelled.
  bool AttemptTimeoutIsCancelledForTesting() const;

  // Record detection stats such as detection duration and detection
  // result in UMA.
  void RecordDetectionStats(const NetworkState* network,
                            CaptivePortalStatus status);

  // Sets current test time ticks. Used by unit tests.
  void set_time_ticks_for_testing(const base::TimeTicks& time_ticks) {
    time_ticks_for_testing_ = time_ticks;
  }

  // Advances current test time ticks. Used by unit tests.
  void advance_time_ticks_for_testing(const base::TimeDelta& delta) {
    time_ticks_for_testing_ += delta;
  }

  // Name of the default network.
  std::string default_network_name_;

  // Unique identifier of the default network.
  std::string default_network_id_;

  // Service path of the default network.
  std::string default_service_path_;

  // Connection state of the default network.
  std::string default_connection_state_;

  State state_;
  CaptivePortalStateMap portal_state_map_;
  ObserverList<Observer> observers_;

  base::CancelableClosure attempt_task_;
  base::CancelableClosure attempt_timeout_;

  // URL that returns a 204 response code when connected to the Internet.
  GURL test_url_;

  // Detector for checking default network for a portal state.
  scoped_ptr<captive_portal::CaptivePortalDetector> captive_portal_detector_;

  // True if the NetworkPortalDetector is enabled.
  bool enabled_;

  base::WeakPtrFactory<NetworkPortalDetectorImpl> weak_factory_;

  // Start time of portal detection.
  base::TimeTicks detection_start_time_;

  // Start time of detection attempt.
  base::TimeTicks attempt_start_time_;

  // Number of already performed detection attempts.
  int attempt_count_;

  // Delay before next portal detection.
  base::TimeDelta next_attempt_delay_;

  // Current detection strategy.
  scoped_ptr<PortalDetectorStrategy> strategy_;

  // UI notification controller about captive portal state.
  NetworkPortalNotificationController notification_controller_;

  content::NotificationRegistrar registrar_;

  // Test time ticks used by unit tests.
  base::TimeTicks time_ticks_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(NetworkPortalDetectorImpl);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_NET_NETWORK_PORTAL_DETECTOR_IMPL_H_
