#!/usr/bin/env python3
"""
掃描 BLE 設備並顯示原始廣告數據
"""

import asyncio
from bleak import BleakScanner

async def main():
    print("掃描 BLE 設備並顯示廣告數據...")
    print("=" * 100)

    scanner = BleakScanner()
    devices = await scanner.discover(timeout=10.0, return_adv=True)

    found_target = False
    for i, device_info in enumerate(devices):
        # 處理不同的返回格式
        if isinstance(device_info, tuple) and len(device_info) == 2:
            device, adv_data = device_info
        else:
            # 可能是舊版本或其他格式
            print(f"未知格式: {type(device_info)}")
            continue
        
        addr = device.address
        name = device.name

        # 尋找目標設備或顯示所有設備的名稱信息
        if "9c:13:9e:ac:32:fd" in addr.lower():
            print(f"\n找到目標設備！ {i+1}. {addr}")
            found_target = True
        elif i < 15:  # 只顯示前 15 個設備
            print(f"\n設備 {i+1}. {addr}")

        # 顯示名稱信息
        print(f"  設備物件名稱: {name if name else '(None)'}")

        # 顯示廣告數據中的名稱信息
        if hasattr(adv_data, 'local_name'):
            print(f"  local_name: {adv_data.local_name}")
        if hasattr(adv_data, 'complete_local_name'):
            print(f"  complete_local_name: {adv_data.complete_local_name}")
        if hasattr(adv_data, 'shortened_local_name'):
            print(f"  shortened_local_name: {adv_data.shortened_local_name}")

        # 顯示原始廣告數據
        if hasattr(adv_data, 'advertising_data'):
            ad_dict = adv_data.advertising_data
            if ad_dict:
                print(f"  廣告數據 AD Types: {list(ad_dict.keys())}")
                for ad_type, ad_value in ad_dict.items():
                    try:
                        # 嘗試解碼為字符串
                        if ad_type in [8, 9]:  # LOCAL_NAME types
                            name_str = ad_value.decode('utf-8', errors='ignore')
                            print(f"    AD Type {ad_type}: {name_str} (raw: {ad_value.hex()})")
                        elif ad_type == 1:  # FLAGS
                            print(f"    AD Type {ad_type} (FLAGS): {ad_value.hex()}")
                        elif ad_type == 3 or ad_type == 7:  # SERVICE UUIDs
                            print(f"    AD Type {ad_type} (SERVICE UUID): {ad_value.hex()}")
                    except Exception as e:
                        print(f"    AD Type {ad_type}: {ad_value.hex()}")
            else:
                print(f"  廣告數據: (空)")

        # 如果找到目標，停止
        if found_target:
            break
        if i >= 14:
            break

    print("\n" + "=" * 100)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n中斷")
    except Exception as e:
        print(f"錯誤: {e}")
        import traceback
        traceback.print_exc()
