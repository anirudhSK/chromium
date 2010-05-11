// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/image_loading_tracker.h"

#include "base/file_util.h"
#include "chrome/browser/chrome_thread.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_resource.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/notification_type.h"
#include "skia/ext/image_operations.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "webkit/glue/image_decoder.h"

////////////////////////////////////////////////////////////////////////////////
// ImageLoadingTracker::ImageLoader

// A RefCounted class for loading images on the File thread and reporting back
// on the UI thread.
class ImageLoadingTracker::ImageLoader
    : public base::RefCountedThreadSafe<ImageLoader> {
 public:
  explicit ImageLoader(ImageLoadingTracker* tracker)
      : tracker_(tracker) {
    CHECK(ChromeThread::GetCurrentThreadIdentifier(&callback_thread_id_));
    DCHECK(!ChromeThread::CurrentlyOn(ChromeThread::FILE));
  }

  // Lets this class know that the tracker is no longer interested in the
  // results.
  void StopTracking() {
    tracker_ = NULL;
  }

  // Instructs the loader to load a task on the File thread.
  void LoadImage(const ExtensionResource& resource,
                 const gfx::Size& max_size,
                 int id) {
    DCHECK(!ChromeThread::CurrentlyOn(ChromeThread::FILE));
    ChromeThread::PostTask(
        ChromeThread::FILE, FROM_HERE,
        NewRunnableMethod(this, &ImageLoader::LoadOnFileThread, resource,
                          max_size, id));
  }

  void LoadOnFileThread(ExtensionResource resource,
                        const gfx::Size& max_size,
                        int id) {
    DCHECK(ChromeThread::CurrentlyOn(ChromeThread::FILE));

    // Read the file from disk.
    std::string file_contents;
    FilePath path = resource.GetFilePath();
    if (path.empty() || !file_util::ReadFileToString(path, &file_contents)) {
      ReportBack(NULL, resource, id);
      return;
    }

    // Decode the image using WebKit's image decoder.
    const unsigned char* data =
        reinterpret_cast<const unsigned char*>(file_contents.data());
    webkit_glue::ImageDecoder decoder;
    scoped_ptr<SkBitmap> decoded(new SkBitmap());
    *decoded = decoder.Decode(data, file_contents.length());
    if (decoded->empty()) {
      ReportBack(NULL, resource, id);
      return;  // Unable to decode.
    }

    if (decoded->width() > max_size.width() ||
        decoded->height() > max_size.height()) {
      // The bitmap is too big, re-sample.
      *decoded = skia::ImageOperations::Resize(
          *decoded, skia::ImageOperations::RESIZE_LANCZOS3,
          max_size.width(), max_size.height());
    }

    ReportBack(decoded.release(), resource, id);
  }

  void ReportBack(SkBitmap* image, const ExtensionResource& resource,
                  int id) {
    DCHECK(ChromeThread::CurrentlyOn(ChromeThread::FILE));

    ChromeThread::PostTask(
        callback_thread_id_, FROM_HERE,
        NewRunnableMethod(this, &ImageLoader::ReportOnUIThread,
                          image, resource, id));
  }

  void ReportOnUIThread(SkBitmap* image, ExtensionResource resource,
                        int id) {
    DCHECK(!ChromeThread::CurrentlyOn(ChromeThread::FILE));

    if (tracker_)
      tracker_->OnImageLoaded(image, resource, id);

    delete image;
  }

 private:
  // The tracker we are loading the image for. If NULL, it means the tracker is
  // no longer interested in the reply.
  ImageLoadingTracker* tracker_;

  // The thread that we need to call back on to report that we are done.
  ChromeThread::ID callback_thread_id_;

  DISALLOW_COPY_AND_ASSIGN(ImageLoader);
};

////////////////////////////////////////////////////////////////////////////////
// ImageLoadingTracker

ImageLoadingTracker::ImageLoadingTracker(Observer* observer)
    : observer_(observer),
      next_id_(0) {
  registrar_.Add(this, NotificationType::EXTENSION_UNLOADED,
                 NotificationService::AllSources());
  registrar_.Add(this, NotificationType::EXTENSION_UNLOADED_DISABLED,
                 NotificationService::AllSources());
}

ImageLoadingTracker::~ImageLoadingTracker() {
  // The loader is created lazily and is NULL if the tracker is destroyed before
  // any valid image load tasks have been posted.
  if (loader_)
    loader_->StopTracking();
}

void ImageLoadingTracker::LoadImage(Extension* extension,
                                    const ExtensionResource& resource,
                                    const gfx::Size& max_size,
                                    CacheParam cache) {
  // If we don't have a path we don't need to do any further work, just respond
  // back.
  int id = next_id_++;
  if (resource.relative_path().empty()) {
    OnImageLoaded(NULL, resource, id);
    return;
  }

  DCHECK(extension->path() == resource.extension_root());

  // See if the extension has the image already.
  if (extension->HasCachedImage(resource)) {
    SkBitmap image = extension->GetCachedImage(resource);
    OnImageLoaded(&image, resource, id);
    return;
  }

  if (cache == CACHE) {
    load_map_[id] = extension;
  }

  // Instruct the ImageLoader to load this on the File thread. LoadImage does
  // not block.
  if (!loader_)
    loader_ = new ImageLoader(this);
  loader_->LoadImage(resource, max_size, id);
}

void ImageLoadingTracker::OnImageLoaded(
    SkBitmap* image,
    const ExtensionResource& resource,
    int id) {
  LoadMap::iterator i = load_map_.find(id);
  if (i != load_map_.end()) {
    i->second->SetCachedImage(resource, image ? *image : SkBitmap());
    load_map_.erase(i);
  }

  observer_->OnImageLoaded(image, resource, id);
}

void ImageLoadingTracker::Observe(NotificationType type,
                                  const NotificationSource& source,
                                  const NotificationDetails& details) {
  DCHECK(type == NotificationType::EXTENSION_UNLOADED ||
         type == NotificationType::EXTENSION_UNLOADED_DISABLED);

  Extension* extension = Details<Extension>(details).ptr();

  // Remove all entries in the load_map_ referencing the extension. This ensures
  // we don't attempt to cache the image when the load completes.
  for (LoadMap::iterator i = load_map_.begin(); i != load_map_.end();) {
    if (i->second == extension) {
      load_map_.erase(i++);
    } else {
      ++i;
    }
  }
}
