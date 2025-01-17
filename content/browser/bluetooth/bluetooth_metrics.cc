// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/bluetooth/bluetooth_metrics.h"

#include <stdint.h>

#include <map>
#include <set>
#include "base/hash.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/sparse_histogram.h"
#include "content/common/bluetooth/bluetooth_scan_filter.h"
#include "device/bluetooth/bluetooth_uuid.h"

using device::BluetoothUUID;

namespace {

// Generates a hash from a canonical UUID string suitable for
// UMA_HISTOGRAM_SPARSE_SLOWLY (positive int).
//
// Hash values can be produced manually using tool: bluetooth_metrics_hash.
int HashUUID(const std::string& canonical_uuid) {
  DCHECK(canonical_uuid == BluetoothUUID(canonical_uuid).canonical_value());

  // TODO(520284): Other than verifying that uuid is canonical, this logic
  // should be migrated to a dedicated histogram macro for hashed strings.
  uint32_t data =
      base::SuperFastHash(canonical_uuid.data(), canonical_uuid.size());

  // Strip off the sign bit because UMA doesn't support negative values,
  // but takes a signed int as input.
  return static_cast<int>(data & 0x7fffffff);
}
}  // namespace

namespace content {

// General

void RecordWebBluetoothFunctionCall(UMAWebBluetoothFunction function) {
  UMA_HISTOGRAM_ENUMERATION("Bluetooth.Web.FunctionCall.Count",
                            static_cast<int>(function),
                            static_cast<int>(UMAWebBluetoothFunction::COUNT));
}

// requestDevice()

void RecordRequestDeviceOutcome(UMARequestDeviceOutcome outcome) {
  UMA_HISTOGRAM_ENUMERATION("Bluetooth.Web.RequestDevice.Outcome",
                            static_cast<int>(outcome),
                            static_cast<int>(UMARequestDeviceOutcome::COUNT));
}

static void RecordRequestDeviceFilters(
    const std::vector<content::BluetoothScanFilter>& filters) {
  UMA_HISTOGRAM_COUNTS_100("Bluetooth.Web.RequestDevice.Filters.Count",
                           filters.size());
  for (const content::BluetoothScanFilter& filter : filters) {
    UMA_HISTOGRAM_COUNTS_100("Bluetooth.Web.RequestDevice.FilterSize",
                             filter.services.size());
    for (const BluetoothUUID& service : filter.services) {
      // TODO(ortuno): Use a macro to histogram strings.
      // http://crbug.com/520284
      UMA_HISTOGRAM_SPARSE_SLOWLY(
          "Bluetooth.Web.RequestDevice.Filters.Services",
          HashUUID(service.canonical_value()));
    }
  }
}

static void RecordRequestDeviceOptionalServices(
    const std::vector<BluetoothUUID>& optional_services) {
  UMA_HISTOGRAM_COUNTS_100("Bluetooth.Web.RequestDevice.OptionalServices.Count",
                           optional_services.size());
  for (const BluetoothUUID& service : optional_services) {
    // TODO(ortuno): Use a macro to histogram strings.
    // http://crbug.com/520284
    UMA_HISTOGRAM_SPARSE_SLOWLY(
        "Bluetooth.Web.RequestDevice.OptionalServices.Services",
        HashUUID(service.canonical_value()));
  }
}

static void RecordUnionOfServices(
    const std::vector<content::BluetoothScanFilter>& filters,
    const std::vector<BluetoothUUID>& optional_services) {
  std::set<BluetoothUUID> union_of_services(optional_services.begin(),
                                            optional_services.end());

  for (const content::BluetoothScanFilter& filter : filters)
    union_of_services.insert(filter.services.begin(), filter.services.end());

  UMA_HISTOGRAM_COUNTS_100("Bluetooth.Web.RequestDevice.UnionOfServices.Count",
                           union_of_services.size());
}

void RecordRequestDeviceArguments(
    const std::vector<content::BluetoothScanFilter>& filters,
    const std::vector<device::BluetoothUUID>& optional_services) {
  RecordRequestDeviceFilters(filters);
  RecordRequestDeviceOptionalServices(optional_services);
  RecordUnionOfServices(filters, optional_services);
}

// GATTServer.Connect

void RecordConnectGATTOutcome(UMAConnectGATTOutcome outcome) {
  UMA_HISTOGRAM_ENUMERATION("Bluetooth.Web.ConnectGATT.Outcome",
                            static_cast<int>(outcome),
                            static_cast<int>(UMAConnectGATTOutcome::COUNT));
}

void RecordConnectGATTOutcome(CacheQueryOutcome outcome) {
  DCHECK(outcome == CacheQueryOutcome::NO_DEVICE);
  RecordConnectGATTOutcome(UMAConnectGATTOutcome::NO_DEVICE);
}

void RecordConnectGATTTimeSuccess(const base::TimeDelta& duration) {
  UMA_HISTOGRAM_MEDIUM_TIMES("Bluetooth.Web.ConnectGATT.TimeSuccess", duration);
}

void RecordConnectGATTTimeFailed(const base::TimeDelta& duration) {
  UMA_HISTOGRAM_MEDIUM_TIMES("Bluetooth.Web.ConnectGATT.TimeFailed", duration);
}

// getPrimaryService

void RecordGetPrimaryServiceService(const BluetoothUUID& service) {
  // TODO(ortuno): Use a macro to histogram strings.
  // http://crbug.com/520284
  UMA_HISTOGRAM_SPARSE_SLOWLY("Bluetooth.Web.GetPrimaryService.Services",
                              HashUUID(service.canonical_value()));
}

void RecordGetPrimaryServiceOutcome(UMAGetPrimaryServiceOutcome outcome) {
  UMA_HISTOGRAM_ENUMERATION(
      "Bluetooth.Web.GetPrimaryService.Outcome", static_cast<int>(outcome),
      static_cast<int>(UMAGetPrimaryServiceOutcome::COUNT));
}

void RecordGetPrimaryServiceOutcome(CacheQueryOutcome outcome) {
  DCHECK(outcome == CacheQueryOutcome::NO_DEVICE);
  RecordGetPrimaryServiceOutcome(UMAGetPrimaryServiceOutcome::NO_DEVICE);
}

// getCharacteristic & getCharacteristics

void RecordGetCharacteristicsOutcome(
    blink::mojom::WebBluetoothGATTQueryQuantity quantity,
    UMAGetCharacteristicOutcome outcome) {
  switch (quantity) {
    case blink::mojom::WebBluetoothGATTQueryQuantity::SINGLE:
      UMA_HISTOGRAM_ENUMERATION(
          "Bluetooth.Web.GetCharacteristic.Outcome", static_cast<int>(outcome),
          static_cast<int>(UMAGetCharacteristicOutcome::COUNT));
      return;
    case blink::mojom::WebBluetoothGATTQueryQuantity::MULTIPLE:
      UMA_HISTOGRAM_ENUMERATION(
          "Bluetooth.Web.GetCharacteristics.Outcome", static_cast<int>(outcome),
          static_cast<int>(UMAGetCharacteristicOutcome::COUNT));
      return;
  }
}

void RecordGetCharacteristicsOutcome(
    blink::mojom::WebBluetoothGATTQueryQuantity quantity,
    CacheQueryOutcome outcome) {
  switch (outcome) {
    case CacheQueryOutcome::SUCCESS:
    case CacheQueryOutcome::BAD_RENDERER:
      // No need to record a success or renderer crash.
      NOTREACHED();
      return;
    case CacheQueryOutcome::NO_DEVICE:
      RecordGetCharacteristicsOutcome(quantity,
                                      UMAGetCharacteristicOutcome::NO_DEVICE);
      return;
    case CacheQueryOutcome::NO_SERVICE:
      RecordGetCharacteristicsOutcome(quantity,
                                      UMAGetCharacteristicOutcome::NO_SERVICE);
      return;
    case CacheQueryOutcome::NO_CHARACTERISTIC:
      NOTREACHED();
      return;
  }
}

void RecordGetCharacteristicsCharacteristic(
    blink::mojom::WebBluetoothGATTQueryQuantity quantity,
    const std::string& characteristic) {
  switch (quantity) {
    case blink::mojom::WebBluetoothGATTQueryQuantity::SINGLE:
      UMA_HISTOGRAM_SPARSE_SLOWLY(
          "Bluetooth.Web.GetCharacteristic.Characteristic",
          HashUUID(characteristic));
      return;
    case blink::mojom::WebBluetoothGATTQueryQuantity::MULTIPLE:
      UMA_HISTOGRAM_SPARSE_SLOWLY(
          "Bluetooth.Web.GetCharacteristics.Characteristic",
          HashUUID(characteristic));
      return;
  }
}

// GATT Operations

void RecordGATTOperationOutcome(UMAGATTOperation operation,
                                UMAGATTOperationOutcome outcome) {
  switch (operation) {
    case UMAGATTOperation::CHARACTERISTIC_READ:
      RecordCharacteristicReadValueOutcome(outcome);
      return;
    case UMAGATTOperation::CHARACTERISTIC_WRITE:
      RecordCharacteristicWriteValueOutcome(outcome);
      return;
    case UMAGATTOperation::START_NOTIFICATIONS:
      RecordStartNotificationsOutcome(outcome);
      return;
    case UMAGATTOperation::COUNT:
      NOTREACHED();
      return;
  }
  NOTREACHED();
}

static UMAGATTOperationOutcome TranslateCacheQueryOutcomeToGATTOperationOutcome(
    CacheQueryOutcome outcome) {
  switch (outcome) {
    case CacheQueryOutcome::SUCCESS:
    case CacheQueryOutcome::BAD_RENDERER:
      // No need to record a success or renderer crash.
      NOTREACHED();
      return UMAGATTOperationOutcome::NOT_SUPPORTED;
    case CacheQueryOutcome::NO_DEVICE:
      return UMAGATTOperationOutcome::NO_DEVICE;
    case CacheQueryOutcome::NO_SERVICE:
      return UMAGATTOperationOutcome::NO_SERVICE;
    case CacheQueryOutcome::NO_CHARACTERISTIC:
      return UMAGATTOperationOutcome::NO_CHARACTERISTIC;
  }
  NOTREACHED() << "No need to record success or renderer crash";
  return UMAGATTOperationOutcome::NOT_SUPPORTED;
}

// Characteristic.readValue

// static
void RecordCharacteristicReadValueOutcome(UMAGATTOperationOutcome outcome) {
  UMA_HISTOGRAM_ENUMERATION("Bluetooth.Web.Characteristic.ReadValue.Outcome",
                            static_cast<int>(outcome),
                            static_cast<int>(UMAGATTOperationOutcome::COUNT));
}

void RecordCharacteristicReadValueOutcome(CacheQueryOutcome outcome) {
  RecordCharacteristicReadValueOutcome(
      TranslateCacheQueryOutcomeToGATTOperationOutcome(outcome));
}

// Characteristic.writeValue

void RecordCharacteristicWriteValueOutcome(UMAGATTOperationOutcome outcome) {
  UMA_HISTOGRAM_ENUMERATION("Bluetooth.Web.Characteristic.WriteValue.Outcome",
                            static_cast<int>(outcome),
                            static_cast<int>(UMAGATTOperationOutcome::COUNT));
}

void RecordCharacteristicWriteValueOutcome(CacheQueryOutcome outcome) {
  RecordCharacteristicWriteValueOutcome(
      TranslateCacheQueryOutcomeToGATTOperationOutcome(outcome));
}

// Characteristic.startNotifications
void RecordStartNotificationsOutcome(UMAGATTOperationOutcome outcome) {
  UMA_HISTOGRAM_ENUMERATION(
      "Bluetooth.Web.Characteristic.StartNotifications.Outcome",
      static_cast<int>(outcome),
      static_cast<int>(UMAGATTOperationOutcome::COUNT));
}

void RecordStartNotificationsOutcome(CacheQueryOutcome outcome) {
  RecordStartNotificationsOutcome(
      TranslateCacheQueryOutcomeToGATTOperationOutcome(outcome));
}

}  // namespace content
