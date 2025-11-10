#!/usr/bin/env python3
"""
詳細的 BLE 掃描腳本，顯示廣告數據的內容
"""

import asyncio
import sys
from bleak import BleakScanner

async def main():
    print("掃描 BLE 設備並顯示廣告數據...")
    print("=" * 80)

    try:
        # 嘗試使用新版本 bleak API
        scanner = BleakScanner()
        devices = await scanner.discover(timeout=10.0, return_adv=True)
    except TypeError:
        # 舊版本 bleak
        devices = await BleakScanner.discover(timeout=10.0)

    device_count = 0
    for device in devices:
        if isinstance(device, tuple) and len(device) == 2:
            device_obj, adv_data = device
            print(f"\n設備 {device_count + 1}:")
            print(f"  地址: {device_obj.address}")
            print(f"  名稱: {device_obj.name if device_obj.name else '(無名稱)'}")
            print(f"  RSSI: {device_obj.rssi if hasattr(device_obj, 'rssi') else 'N/A'} dBm")

            # 顯示廣告數據內容
            if hasattr(adv_data, 'local_name') and adv_data.local_name:
                print(f"  廣告本地名稱: {adv_data.local_name}")
            if hasattr(adv_data, 'complete_local_name') and adv_data.complete_local_name:
                print(f"  廣告完整名稱: {adv_data.complete_local_name}")
            if hasattr(adv_data, 'manufacturer_data'):
                print(f"  製造商數據: {adv_data.manufacturer_data}")
            if hasattr(adv_data, 'service_uuids'):
                print(f"  服務 UUID: {adv_data.service_uuids}")

            # 原始廣告數據
            if hasattr(adv_data, 'advertising_data'):
                print(f"  原始廣告數據: {adv_data.advertising_data}")

            device_count += 1

            # 只顯示前 10 個設備
            if device_count >= 10:
                break
        else:
            # 舊版本格式
            print(f"\n設備 {device_count + 1}:")
            print(f"  地址: {device.address if hasattr(device, 'address') else 'N/A'}")
            print(f"  名稱: {device.name if hasattr(device, 'name') else '(無名稱)'}")
            device_count += 1
            if device_count >= 10:
                break

    print("\n" + "=" * 80)
    print(f"總共找到 {len(devices)} 個設備，顯示了前 {min(device_count, 10)} 個")

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n\n中斷")
        sys.exit(0)
    except Exception as e:
        print(f"錯誤: {e}")
        sys.exit(1)
