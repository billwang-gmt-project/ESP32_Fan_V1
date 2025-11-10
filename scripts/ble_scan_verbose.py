#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Verbose BLE scanner to dump addresses, names, RSSI and advertisement data.
Run as:
  python .\scripts\ble_scan_verbose.py

This uses Bleak's `return_adv=True` (if available) to show adv payloads.
"""
import asyncio
import sys

if sys.platform == 'win32':
    try:
        sys.stdout.reconfigure(encoding='utf-8')
    except Exception:
        pass

async def main(timeout: float = 15.0):
    try:
        from bleak import BleakScanner
    except ImportError:
        print("Bleak not installed. Install with: python -m pip install bleak")
        return

    print(f"Scanning for BLE devices (timeout={timeout}s, verbose)...")
    print("=" * 60)

    try:
        devices = await BleakScanner.discover(timeout=timeout, return_adv=True)
    except TypeError:
        # Older bleak might not support return_adv; fall back
        print("Warning: Bleak version may not support return_adv=True; falling back to discover(timeout)")
        from bleak import BleakScanner as _S
        devices = await _S.discover(timeout=timeout)

    if not devices:
        print("No BLE devices found.")
        return

    print(f"Found {len(devices)} device(s):")
    print("-" * 60)
    for idx, item in enumerate(devices, 1):
        try:
            if isinstance(item, tuple):
                dev, adv = item
            else:
                dev = item
                adv = None

            name = getattr(dev, 'name', None) or "(None)"
            addr = getattr(dev, 'address', str(dev))
            rssi = getattr(dev, 'rssi', None)

            print(f"{idx}. Address: {addr}")
            print(f"   Name: {name}")
            print(f"   RSSI: {rssi}")
            if adv is not None:
                # adv is platform-specific object; show repr
                print(f"   AdvData: {repr(adv)}")
            print()
        except Exception as e:
            print(f"  ? Failed to parse device #{idx}: {e}")

    print("=" * 60)
    print("Scan complete.")

if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nScan interrupted by user")
    except Exception as e:
        print(f"Fatal error: {type(e).__name__}: {e}")
        import traceback
        traceback.print_exc()
