#!/usr/bin/env python3
"""
使用 bleak 掃描 BLE 設備 (1.1.1 版本相容)
"""

import asyncio
from bleak import BleakScanner

async def main():
    print("掃描 BLE 設備...")
    print("=" * 80)

    # 使用 bleak 1.1.1 的正確方式
    devices = await BleakScanner.discover(timeout=10.0)

    print(f"找到 {len(devices)} 個設備\n")

    target_found = False
    for i, device in enumerate(devices):
        addr = device.address if hasattr(device, 'address') else str(device)
        name = device.name if hasattr(device, 'name') else None

        # 查找目標設備或顯示所有設備
        if "9c:13:9e:ac:32:fd" in addr.lower():
            print(f">>> 找到目標設備！")
            target_found = True

        if target_found or i < 10:  # 顯示前 10 個或目標設備
            print(f"{i+1}. Address: {addr}")
            print(f"   Name: {name if name else '(None)'}")
            if target_found:
                break

    print("\n" + "=" * 80)
    if target_found:
        print("[OK] 成功找到 BillCat_Fan_Control 設備")
    else:
        print("[FAIL] 未找到 BillCat_Fan_Control 設備")

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n中斷")
    except Exception as e:
        print(f"錯誤: {e}")
        import traceback
        traceback.print_exc()
