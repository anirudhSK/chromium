// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_WM_CORE_NATIVE_CURSOR_MANAGER_H_
#define UI_WM_CORE_NATIVE_CURSOR_MANAGER_H_

#include "base/strings/string16.h"
#include "ui/wm/core/native_cursor_manager_delegate.h"
#include "ui/wm/core/wm_core_export.h"

namespace gfx {
class Display;
}

namespace views {
namespace corewm {

// Interface where platforms such as Ash or Desktop aura are notified of
// requested changes to cursor state. When requested, implementer should tell
// the CursorManager of any actual state changes performed through the
// delegate.
class WM_CORE_EXPORT NativeCursorManager {
 public:
  virtual ~NativeCursorManager() {}

  // A request to set the screen DPI. Can cause changes in the current cursor.
  virtual void SetDisplay(
      const gfx::Display& display,
      views::corewm::NativeCursorManagerDelegate* delegate) = 0;

  // A request to set the cursor to |cursor|. At minimum, implementer should
  // call NativeCursorManagerDelegate::CommitCursor() with whatever cursor is
  // actually used.
  virtual void SetCursor(
      gfx::NativeCursor cursor,
      views::corewm::NativeCursorManagerDelegate* delegate) = 0;

  // A request to set the visibility of the cursor. At minimum, implementer
  // should call NativeCursorManagerDelegate::CommitVisibility() with whatever
  // the visibility is.
  virtual void SetVisibility(
    bool visible,
    views::corewm::NativeCursorManagerDelegate* delegate) = 0;

  // A request to set the scale of the cursor icon.
  virtual void SetScale(
      float scale,
      views::corewm::NativeCursorManagerDelegate* delegate) = 0;

  // A request to set the scale of the cursor icon.
  virtual void SetCursorSet(
      ui::CursorSetType cursor_set,
      views::corewm::NativeCursorManagerDelegate* delegate) = 0;

  // A request to set whether mouse events are disabled. At minimum,
  // implementer should call NativeCursorManagerDelegate::
  // CommitMouseEventsEnabled() with whether mouse events are actually enabled.
  virtual void SetMouseEventsEnabled(
      bool enabled,
      views::corewm::NativeCursorManagerDelegate* delegate) = 0;
};

}  // namespace corewm
}  // namespace views

#endif  // UI_WM_CORE_NATIVE_CURSOR_MANAGER_H_
