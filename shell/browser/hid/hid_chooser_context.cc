// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/hid/hid_chooser_context.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/grit/generated_resources.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/device_service.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

bool CanStorePersistentEntry(const device::mojom::HidDeviceInfo& device) {
  return !device.serial_number.empty() && !device.product_name.empty();
}

}  // namespace

namespace electron {

void HidChooserContext::DeviceObserver::OnDeviceAdded(
    const device::mojom::HidDeviceInfo& device) {}

void HidChooserContext::DeviceObserver::OnDeviceRemoved(
    const device::mojom::HidDeviceInfo& device) {}

void HidChooserContext::DeviceObserver::OnHidManagerConnectionError() {}

HidChooserContext::HidChooserContext() {}

HidChooserContext::~HidChooserContext() {
  // Notify observers that the chooser context is about to be destroyed.
  // Observers must remove themselves from the observer lists.
  for (auto& observer : device_observer_list_) {
    observer.OnHidChooserContextShutdown();
    DCHECK(!device_observer_list_.HasObserver(&observer));
  }
}

// static
base::string16 HidChooserContext::DisplayNameFromDeviceInfo(
    const device::mojom::HidDeviceInfo& device) {
  auto vendor_id_string =
      base::ASCIIToUTF16(base::StringPrintf("0x%04x", device.vendor_id));
  auto product_id_string =
      base::ASCIIToUTF16(base::StringPrintf("0x%04x", device.product_id));
  if (device.product_name.empty()) {
    return l10n_util::GetStringFUTF16(IDS_HID_CHOOSER_ITEM_WITHOUT_NAME,
                                      vendor_id_string, product_id_string);
  }
  return l10n_util::GetStringFUTF16(IDS_HID_CHOOSER_ITEM_WITH_NAME,
                                    base::UTF8ToUTF16(device.product_name),
                                    vendor_id_string, product_id_string);
}

void HidChooserContext::GrantDevicePermission(
    const url::Origin& requesting_origin,
    const url::Origin& embedding_origin,
    const device::mojom::HidDeviceInfo& device) {}

bool HidChooserContext::HasDevicePermission(
    const url::Origin& requesting_origin,
    const url::Origin& embedding_origin,
    const device::mojom::HidDeviceInfo& device) {
  return true;
}

void HidChooserContext::AddDeviceObserver(DeviceObserver* observer) {
  EnsureHidManagerConnection();
  device_observer_list_.AddObserver(observer);
}

void HidChooserContext::RemoveDeviceObserver(DeviceObserver* observer) {
  device_observer_list_.RemoveObserver(observer);
}

void HidChooserContext::GetDevices(
    device::mojom::HidManager::GetDevicesCallback callback) {
  if (!is_initialized_) {
    EnsureHidManagerConnection();
    pending_get_devices_requests_.push(std::move(callback));
    return;
  }

  std::vector<device::mojom::HidDeviceInfoPtr> device_list;
  device_list.reserve(devices_.size());
  for (const auto& pair : devices_)
    device_list.push_back(pair.second->Clone());
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(device_list)));
}

const device::mojom::HidDeviceInfo* HidChooserContext::GetDeviceInfo(
    const std::string& guid) {
  DCHECK(is_initialized_);
  auto it = devices_.find(guid);
  return it == devices_.end() ? nullptr : it->second.get();
}

device::mojom::HidManager* HidChooserContext::GetHidManager() {
  EnsureHidManagerConnection();
  return hid_manager_.get();
}

void HidChooserContext::SetHidManagerForTesting(
    mojo::PendingRemote<device::mojom::HidManager> manager,
    device::mojom::HidManager::GetDevicesCallback callback) {
  hid_manager_.Bind(std::move(manager));
  hid_manager_.set_disconnect_handler(base::BindOnce(
      &HidChooserContext::OnHidManagerConnectionError, base::Unretained(this)));

  hid_manager_->GetDevicesAndSetClient(
      client_receiver_.BindNewEndpointAndPassRemote(), std::move(callback));
}

base::WeakPtr<HidChooserContext> HidChooserContext::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void HidChooserContext::DeviceAdded(device::mojom::HidDeviceInfoPtr device) {
  DCHECK(device);

  // Update the device list.
  if (!base::Contains(devices_, device->guid))
    devices_.insert({device->guid, device->Clone()});

  // Notify all observers.
  for (auto& observer : device_observer_list_)
    observer.OnDeviceAdded(*device);
}

void HidChooserContext::DeviceRemoved(device::mojom::HidDeviceInfoPtr device) {
  DCHECK(device);
  DCHECK(base::Contains(devices_, device->guid));

  // Update the device list.
  devices_.erase(device->guid);

  // Notify all device observers.
  for (auto& observer : device_observer_list_)
    observer.OnDeviceRemoved(*device);

  // Next we'll notify observers for revoked permissions. If the device does not
  // support persistent permissions then device permissions are revoked on
  // disconnect.
  if (CanStorePersistentEntry(*device))
    return;

  std::vector<std::pair<url::Origin, url::Origin>> revoked_url_pairs;
  for (auto& map_entry : ephemeral_devices_) {
    if (map_entry.second.erase(device->guid) > 0)
      revoked_url_pairs.push_back(map_entry.first);
  }
  if (revoked_url_pairs.empty())
    return;
}

void HidChooserContext::EnsureHidManagerConnection() {
  if (hid_manager_)
    return;

  mojo::PendingRemote<device::mojom::HidManager> manager;
  content::GetDeviceService().BindHidManager(
      manager.InitWithNewPipeAndPassReceiver());
  SetUpHidManagerConnection(std::move(manager));
}

void HidChooserContext::SetUpHidManagerConnection(
    mojo::PendingRemote<device::mojom::HidManager> manager) {
  hid_manager_.Bind(std::move(manager));
  hid_manager_.set_disconnect_handler(base::BindOnce(
      &HidChooserContext::OnHidManagerConnectionError, base::Unretained(this)));

  hid_manager_->GetDevicesAndSetClient(
      client_receiver_.BindNewEndpointAndPassRemote(),
      base::BindOnce(&HidChooserContext::InitDeviceList,
                     weak_factory_.GetWeakPtr()));
}

void HidChooserContext::InitDeviceList(
    std::vector<device::mojom::HidDeviceInfoPtr> devices) {
  for (auto& device : devices)
    devices_.insert({device->guid, std::move(device)});

  is_initialized_ = true;

  while (!pending_get_devices_requests_.empty()) {
    std::vector<device::mojom::HidDeviceInfoPtr> device_list;
    device_list.reserve(devices.size());
    for (const auto& entry : devices_)
      device_list.push_back(entry.second->Clone());
    std::move(pending_get_devices_requests_.front())
        .Run(std::move(device_list));
    pending_get_devices_requests_.pop();
  }
}

void HidChooserContext::OnHidManagerConnectionError() {
  hid_manager_.reset();
  client_receiver_.reset();
  devices_.clear();

  std::vector<std::pair<url::Origin, url::Origin>> revoked_origins;
  revoked_origins.reserve(ephemeral_devices_.size());
  for (const auto& map_entry : ephemeral_devices_)
    revoked_origins.push_back(map_entry.first);
  ephemeral_devices_.clear();

  // Notify all device observers.
  for (auto& observer : device_observer_list_)
    observer.OnHidManagerConnectionError();
}

}  // namespace electron
