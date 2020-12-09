// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_HID_ELECTRON_HID_DELEGATE_H_
#define SHELL_BROWSER_HID_ELECTRON_HID_DELEGATE_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/hid_delegate.h"
// #include "shell/browser/serial/serial_chooser_controller.h"

namespace electron {

class ElectronHidDelegate : public content::HidDelegate {
 public:
  ~ElectronHidDelegate() override = default;

  // Shows a chooser for the user to select a HID device. |callback| will be
  // run when the prompt is closed. Deleting the returned object will cancel the
  // prompt.
  std::unique_ptr<content::HidChooser> RunChooser(
      content::RenderFrameHost* frame,
      std::vector<blink::mojom::HidDeviceFilterPtr> filters,
      content::HidChooser::Callback callback) override;

  // Returns whether the main frame owned by |web_contents| has permission to
  // request access to a device. |requesting_origin| is the origin of the
  // frame that would make the request.
  bool CanRequestDevicePermission(
      content::WebContents* web_contents,
      const url::Origin& requesting_origin) override;

  // Returns whether the main frame owned by |web_contents| has permission to
  // access |device|. |requesting_origin| is the origin of the frame making
  // the request.
  bool HasDevicePermission(content::WebContents* web_contents,
                           const url::Origin& requesting_origin,
                           const device::mojom::HidDeviceInfo& device) override;

  // Returns an open connection to the HidManager interface owned by the
  // embedder and being used to serve requests from |web_contents|.
  //
  // Content and the embedder must use the same connection so that the embedder
  // can process connect/disconnect events for permissions management purposes
  // before they are delivered to content. Otherwise race conditions are
  // possible.
  device::mojom::HidManager* GetHidManager(
      content::WebContents* web_contents) override;

  // Functions to manage the set of Observer instances registered to this
  // object.
  void AddObserver(content::RenderFrameHost* frame,
                   content::HidDelegate::Observer* observer) override;
  void RemoveObserver(content::RenderFrameHost* frame,
                      content::HidDelegate::Observer* observer) override;

  // Gets the device info for a particular device, identified by its guid.
  const device::mojom::HidDeviceInfo* GetDeviceInfo(
      content::WebContents* web_contents,
      const std::string& guid) override;
};

}  // namespace electron

#endif