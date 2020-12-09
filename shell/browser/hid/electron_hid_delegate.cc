#include "electron_hid_delegate.h"

#include "base/callback.h"
#include "content/public/browser/web_contents.h"
#include "shell/browser/hid/hid_chooser_context.h"
#include "shell/browser/hid/hid_chooser_context_factory.h"

namespace electron {
std::unique_ptr<content::HidChooser> ElectronHidDelegate::RunChooser(
    content::RenderFrameHost* frame,
    std::vector<blink::mojom::HidDeviceFilterPtr> filters,
    content::HidChooser::Callback callback) {
  std::move(callback).Run({});
  return nullptr;
}

// Returns whether the main frame owned by |web_contents| has permission to
// request access to a device. |requesting_origin| is the origin of the
// frame that would make the request.
bool ElectronHidDelegate::CanRequestDevicePermission(
    content::WebContents* web_contents,
    const url::Origin& requesting_origin) {
  return true;
}

// Returns whether the main frame owned by |web_contents| has permission to
// access |device|. |requesting_origin| is the origin of the frame making
// the request.
bool ElectronHidDelegate::HasDevicePermission(
    content::WebContents* web_contents,
    const url::Origin& requesting_origin,
    const device::mojom::HidDeviceInfo& device) {
  return true;
}

// Returns an open connection to the HidManager interface owned by the
// embedder and being used to serve requests from |web_contents|.
//
// Content and the embedder must use the same connection so that the embedder
// can process connect/disconnect events for permissions management purposes
// before they are delivered to content. Otherwise race conditions are
// possible.
device::mojom::HidManager* ElectronHidDelegate::GetHidManager(
    content::WebContents* web_contents) {
  return HidChooserContextFactory::GetForBrowserContext(
             web_contents->GetBrowserContext())
      ->GetHidManager();
}

// Functions to manage the set of Observer instances registered to this
// object.
void ElectronHidDelegate::AddObserver(
    content::RenderFrameHost* frame,
    content::HidDelegate::Observer* observer) {}
void ElectronHidDelegate::RemoveObserver(
    content::RenderFrameHost* frame,
    content::HidDelegate::Observer* observer) {}

// Gets the device info for a particular device, identified by its guid.
const device::mojom::HidDeviceInfo* ElectronHidDelegate::GetDeviceInfo(
    content::WebContents* web_contents,
    const std::string& guid) {
  return nullptr;
}
}  // namespace electron