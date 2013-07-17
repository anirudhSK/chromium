// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_MEDIA_STREAM_CENTER_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_MEDIA_STREAM_CENTER_HOST_H_

#include <map>
#include <string>

#include "base/synchronization/lock.h"
#include "content/browser/renderer_host/media/media_stream_requester.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_message_filter.h"

namespace content {

class MediaStreamManager;

// DeviceRequestMessageFilter used to delegate requests from the
// MediaStreamCenter.
class CONTENT_EXPORT DeviceRequestMessageFilter : public BrowserMessageFilter,
                                                  public MediaStreamRequester {
 public:
  explicit DeviceRequestMessageFilter(MediaStreamManager* media_stream_manager);

  // MediaStreamRequester implementation.
  // TODO(vrk): Replace MediaStreamRequester interface with a single callback so
  // we don't have to override all these callbacks we don't care about.
  // (crbug.com/249476)
  virtual void StreamGenerated(
      const std::string& label, const StreamDeviceInfoArray& audio_devices,
      const StreamDeviceInfoArray& video_devices) OVERRIDE;
  virtual void StreamGenerationFailed(const std::string& label) OVERRIDE;
  virtual void DeviceOpened(const std::string& label,
                            const StreamDeviceInfo& video_device) OVERRIDE;
  // DevicesEnumerated() is the only callback we're interested in.
  virtual void DevicesEnumerated(const std::string& label,
                                 const StreamDeviceInfoArray& devices) OVERRIDE;

  // BrowserMessageFilter implementation.
  virtual bool OnMessageReceived(const IPC::Message& message,
                                 bool* message_was_ok) OVERRIDE;
  virtual void OnChannelClosing() OVERRIDE;

  // Helper method that checks whether the GUID generated by
  // DeviceRequestMessageFilter matches the given |raw_device_id|.
  static bool DoesRawIdMatchGuid(const GURL& security_origin,
                                 const std::string& device_guid,
                                 const std::string& raw_device_id);

 protected:
  virtual ~DeviceRequestMessageFilter();

 private:
  void OnGetSources(int request_id, const GURL& security_origin);
  void HmacDeviceIds(const GURL& origin,
                     const StreamDeviceInfoArray& raw_devices,
                     StreamDeviceInfoArray* devices_with_guids);

  MediaStreamManager* media_stream_manager_;

  struct DeviceRequest;
  typedef std::vector<DeviceRequest> DeviceRequestList;
  // List of all requests.
  DeviceRequestList requests_;

  DISALLOW_COPY_AND_ASSIGN(DeviceRequestMessageFilter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_MEDIA_STREAM_CENTER_HOST_H_
