# Changelog

All notable changes to the ESP32-S3 USB Composite Device project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project uses semantic versioning.

---

## [Unreleased]

### Documentation
- Add comprehensive documentation review report (DOCUMENTATION_REVIEW.md)
- Simplify board variants section in CLAUDE.md with detailed appendix
- Fix hardcoded line numbers in CLAUDE.md for better maintainability
- Add language policy note to README.md
- Add MIT LICENSE file
- Add this CHANGELOG.md

---

## [2.1.0] - 2025-01

### Changed
- **Removed character echo**: CDC input no longer echoes characters, providing cleaner command responses
- **Smart response routing**:
  - CDC commands → CDC response only (CDCResponse)
  - HID commands → CDC + HID dual-channel response (MultiChannelResponse)
- **Command response optimization**: `*IDN?` now returns only device ID, without echoing the command

### Added
- Plain text HID protocol support (in addition to 0xA1 structured protocol)
- Protocol auto-detection in HIDProtocol::parseCommand()
- Interactive protocol switching in test scripts
- COM port smart filtering in test scripts (skip Bluetooth, virtual ports, non-CDC devices)

### Documentation
- Updated all flow diagrams to reflect actual behavior
- Added detailed response routing tables
- Expanded command list to 4-column table (command/description/response/notes)
- Added quick reference section with key features
- Added FAQ section (5 common questions)
- Enhanced FreeRTOS task architecture documentation
- Added version history tracking in PROTOCOL.md

---

## [2.0.0] - 2024-12

### Added
- **0xA1 protocol implementation**: Structured HID command format with 3-byte header
- **Multi-channel response system**: Simultaneous CDC and HID responses for HID-originated commands
- **Protocol-aware packet handling**: Distinguish command packets (0xA1) from raw data
- **FreeRTOS mutex protection**: Thread-safe HID.send() operations
- **Dual protocol support**: Both 0xA1 structured and plain text protocols

### Changed
- HID responses now include 3-byte header: `[0xA1][length][0x00][data]`
- Long responses automatically split into multiple 64-byte packets
- Command and raw data processing now separated in HID task

### Implementation Details
- Added HIDProtocol class for packet encoding/decoding
- Added MultiChannelResponse class for dual-channel output
- Enhanced command parser with source tracking (CDC vs HID)
- Added protocol type display in debug output

---

## [1.0.0] - 2024-11

### Added
- Initial release of ESP32-S3 USB composite device
- **Dual-interface design**: CDC (Serial) + Custom HID (64-byte)
- **Unified command system**: 7 commands accessible from both interfaces
  - `*IDN?` - Device identification
  - `HELP` - Command list
  - `INFO` - Device information
  - `STATUS` - System status
  - `SEND` - Send test HID report
  - `READ` - Read HID buffer
  - `CLEAR` - Clear HID buffer
- **FreeRTOS multi-tasking architecture**:
  - hidTask (Priority 2) - HID data processing
  - cdcTask (Priority 1) - CDC serial processing
- **Thread-safe design**: Mutexes for USBSerial, HID buffer, and HID send operations
- **64-byte HID implementation**: Custom HID class without Report ID
- **Test scripts**: Python-based testing tools (test_hid.py, test_cdc.py, test_all.py)

### Features
- Simple string parsing (character-by-character scanning for `\n`)
- Character echo on all interfaces
- Response sent only to source interface
- Queue-based HID data transfer (10-packet deep queue)
- ISR-driven HID data reception with task notification

### Hardware Support
- ESP32-S3-DevKitC-1 N16R8 (16MB Flash, 8MB PSRAM)
- Support for other ESP32-S3-DevKitC-1 variants via platformio.ini configuration

### Documentation
- README.md - Quick start guide (Traditional Chinese)
- PROTOCOL.md - HID protocol specification (Traditional Chinese)
- TESTING.md - Testing guide (Traditional Chinese)
- CLAUDE.md - AI-assisted development guide (English)

---

## Release Notes

### Version Numbering Scheme

This project follows semantic versioning (MAJOR.MINOR.PATCH):
- **MAJOR**: Incompatible protocol or API changes
- **MINOR**: New features, backward-compatible
- **PATCH**: Bug fixes, backward-compatible

### Upgrade Notes

#### From v2.0.0 to v2.1.0
- No breaking changes
- Test scripts updated with protocol switching and COM port filtering
- Documentation improvements only
- Firmware changes: Remove character echo, smart response routing
- **Action required**: If you rely on character echo, you may need to adjust terminal behavior

#### From v1.0.0 to v2.0.0
- **Breaking change**: HID response format changed
  - Old: Plain data without header
  - New: `[0xA1][length][0x00][data]` header format
- **Action required**: Update host-side HID parsing code to handle new format
- CDC interface unchanged, no action required for CDC-only usage
- Test scripts updated to support both protocols

### Migration Guide

#### For Host Applications Using HID

**v1.0.0 Response:**
```
[response_text][padding...]
```

**v2.0.0+ Response:**
```
[0xA1][length][0x00][response_text][padding...]
```

**Migration Steps:**
1. Check first byte: if `0xA1`, it's a command response
2. Read length from second byte
3. Extract data starting from byte 4 (skip 3-byte header)
4. If response is longer than 61 bytes, expect multiple packets

**Example Python Code:**
```python
def parse_hid_response(data):
    if data[0] == 0xA1:  # Command response
        length = data[1]
        payload = data[3:3+length]
        return payload.decode('utf-8')
    else:  # Raw data (v1.0 style or non-command)
        return data
```

---

## Future Roadmap

### Planned Features (v2.2.0)
- [ ] BLE interface support
- [ ] WebSocket control interface
- [ ] Command registration system for dynamic commands
- [ ] Event subscription mechanism
- [ ] Macro command support

### Under Consideration
- [ ] CRC/Checksum for data integrity
- [ ] Packet sequence numbers
- [ ] Error recovery mechanisms
- [ ] Flash-based configuration storage
- [ ] OTA (Over-The-Air) firmware updates

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines (if available).

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
