#include "WiFiManager.h"
#include "USBCDC.h"

// External reference to USBSerial (defined in main.cpp)
extern USBCDC USBSerial;

WiFiManager::WiFiManager() {
    // Constructor
}

bool WiFiManager::begin(WiFiSettings* settings) {
    if (!settings) {
        USBSerial.println("❌ WiFiManager::begin() - NULL settings pointer!");
        return false;
    }

    pSettings = settings;
    status = WiFiStatus::DISCONNECTED;

    USBSerial.println("✅ WiFi Manager initialized");
    return true;
}

bool WiFiManager::start() {
    if (!pSettings) {
        USBSerial.println("❌ WiFi settings not initialized!");
        return false;
    }

    // Stop any existing WiFi connection
    WiFi.mode(WIFI_OFF);
    delay(100);

    bool success = false;

    switch (pSettings->mode) {
        case WiFiMode::OFF:
            USBSerial.println("📡 WiFi mode: OFF");
            status = WiFiStatus::DISCONNECTED;
            success = true;
            break;

        case WiFiMode::AP:
            USBSerial.println("📡 WiFi mode: Access Point");
            success = startAP();
            break;

        case WiFiMode::STA:
            USBSerial.println("📡 WiFi mode: Station");
            success = startStation();
            break;

        case WiFiMode::AP_STA:
            USBSerial.println("📡 WiFi mode: AP + Station");
            // Start AP first
            if (startAP()) {
                // Then try to connect as station
                startStation();
                success = true;
            }
            break;
    }

    return success;
}

void WiFiManager::stop() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    status = WiFiStatus::DISCONNECTED;
    USBSerial.println("📡 WiFi stopped");
}

void WiFiManager::update() {
    if (!pSettings) {
        return;
    }

    unsigned long now = millis();

    // Periodic status check
    if (now - lastStatusCheck >= STATUS_CHECK_INTERVAL_MS) {
        lastStatusCheck = now;

        // Check Station connection status
        if (pSettings->mode == WiFiMode::STA || pSettings->mode == WiFiMode::AP_STA) {
            if (WiFi.status() == WL_CONNECTED) {
                if (status != WiFiStatus::CONNECTED) {
                    status = WiFiStatus::CONNECTED;
                    USBSerial.println("✅ WiFi connected");
                    USBSerial.print("  IP Address: ");
                    USBSerial.println(WiFi.localIP());
                    USBSerial.print("  RSSI: ");
                    USBSerial.print(WiFi.RSSI());
                    USBSerial.println(" dBm");
                }
            } else {
                // Connection lost - attempt reconnect
                if (status == WiFiStatus::CONNECTED) {
                    USBSerial.println("⚠️ WiFi connection lost");
                    status = WiFiStatus::CONNECTING;
                }

                // Reconnect attempt
                if (now - lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
                    lastReconnectAttempt = now;
                    USBSerial.println("🔄 Attempting WiFi reconnect...");
                    WiFi.reconnect();
                }
            }
        }

        // Check AP mode status
        if (pSettings->mode == WiFiMode::AP || pSettings->mode == WiFiMode::AP_STA) {
            if (status == WiFiStatus::AP_STARTED) {
                // AP is running, optionally check client count
                uint8_t clients = WiFi.softAPgetStationNum();
                if (clients > 0) {
                    // Clients connected
                }
            }
        }
    }
}

WiFiStatus WiFiManager::getStatus() const {
    return status;
}

String WiFiManager::getModeString() const {
    if (!pSettings) {
        return "Not initialized";
    }

    switch (pSettings->mode) {
        case WiFiMode::OFF: return "OFF";
        case WiFiMode::AP: return "Access Point";
        case WiFiMode::STA: return "Station";
        case WiFiMode::AP_STA: return "AP + Station";
        default: return "Unknown";
    }
}

String WiFiManager::getIPAddress() const {
    if (!pSettings) {
        return "0.0.0.0";
    }

    switch (pSettings->mode) {
        case WiFiMode::AP:
        case WiFiMode::AP_STA:
            return WiFi.softAPIP().toString();

        case WiFiMode::STA:
            if (WiFi.status() == WL_CONNECTED) {
                return WiFi.localIP().toString();
            }
            return "0.0.0.0";

        default:
            return "0.0.0.0";
    }
}

uint8_t WiFiManager::getClientCount() const {
    if (pSettings && (pSettings->mode == WiFiMode::AP || pSettings->mode == WiFiMode::AP_STA)) {
        return WiFi.softAPgetStationNum();
    }
    return 0;
}

int8_t WiFiManager::getRSSI() const {
    if (pSettings && (pSettings->mode == WiFiMode::STA || pSettings->mode == WiFiMode::AP_STA)) {
        if (WiFi.status() == WL_CONNECTED) {
            return WiFi.RSSI();
        }
    }
    return 0;
}

bool WiFiManager::isConnected() const {
    return (status == WiFiStatus::CONNECTED || status == WiFiStatus::AP_STARTED);
}

bool WiFiManager::startAP() {
    if (!pSettings) {
        return false;
    }

    USBSerial.printf("🔧 Starting Access Point: %s\n", pSettings->ap_ssid);

    // Configure AP mode
    WiFi.mode(WIFI_AP);

    // Start AP
    bool success = WiFi.softAP(
        pSettings->ap_ssid,
        pSettings->ap_password,
        pSettings->ap_channel,
        0,  // SSID hidden (0 = visible)
        4   // Max connections
    );

    if (success) {
        status = WiFiStatus::AP_STARTED;
        USBSerial.println("✅ Access Point started");
        USBSerial.print("  SSID: ");
        USBSerial.println(pSettings->ap_ssid);
        USBSerial.print("  IP Address: ");
        USBSerial.println(WiFi.softAPIP());
        USBSerial.print("  Channel: ");
        USBSerial.println(pSettings->ap_channel);
        return true;
    } else {
        status = WiFiStatus::ERROR;
        USBSerial.println("❌ Failed to start Access Point");
        return false;
    }
}

bool WiFiManager::startStation() {
    if (!pSettings) {
        return false;
    }

    if (strlen(pSettings->sta_ssid) == 0) {
        USBSerial.println("❌ Station SSID not configured");
        return false;
    }

    USBSerial.printf("🔧 Connecting to WiFi: %s\n", pSettings->sta_ssid);

    // Set mode to station
    if (pSettings->mode == WiFiMode::AP_STA) {
        WiFi.mode(WIFI_AP_STA);
    } else {
        WiFi.mode(WIFI_STA);
    }

    // Configure static IP if not using DHCP
    if (!pSettings->sta_dhcp) {
        if (!configureStaticIP()) {
            USBSerial.println("⚠️ Failed to configure static IP, using DHCP");
        }
    }

    // Set hostname
    WiFi.setHostname("ESP32-Motor-Control");

    // Begin connection
    WiFi.begin(pSettings->sta_ssid, pSettings->sta_password);

    status = WiFiStatus::CONNECTING;
    lastReconnectAttempt = millis();

    // Wait for connection (non-blocking)
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < CONNECTION_TIMEOUT_MS) {
        delay(100);
        USBSerial.print(".");
    }
    USBSerial.println();

    if (WiFi.status() == WL_CONNECTED) {
        status = WiFiStatus::CONNECTED;
        USBSerial.println("✅ WiFi connected");
        USBSerial.print("  IP Address: ");
        USBSerial.println(WiFi.localIP());
        USBSerial.print("  Gateway: ");
        USBSerial.println(WiFi.gatewayIP());
        USBSerial.print("  RSSI: ");
        USBSerial.print(WiFi.RSSI());
        USBSerial.println(" dBm");
        return true;
    } else {
        status = WiFiStatus::ERROR;
        USBSerial.println("❌ WiFi connection failed");
        return false;
    }
}

int WiFiManager::scanNetworks(int maxResults) {
    USBSerial.println("🔍 Scanning for WiFi networks...");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    scanResults = WiFi.scanNetworks();

    if (scanResults == 0) {
        USBSerial.println("⚠️ No networks found");
    } else if (scanResults > 0) {
        USBSerial.printf("✅ Found %d networks\n", scanResults);
    } else {
        USBSerial.println("❌ Network scan failed");
    }

    return scanResults;
}

bool WiFiManager::getScanResult(int index, String& ssid, int8_t& rssi, bool& secure) {
    if (scanResults < 0 || index < 0 || index >= scanResults) {
        return false;
    }

    ssid = WiFi.SSID(index);
    rssi = WiFi.RSSI(index);
    secure = (WiFi.encryptionType(index) != WIFI_AUTH_OPEN);

    return true;
}

bool WiFiManager::configureStaticIP() {
    IPAddress ip, gateway, subnet;

    if (!ip.fromString(pSettings->sta_ip)) {
        USBSerial.printf("❌ Invalid IP address: %s\n", pSettings->sta_ip);
        return false;
    }

    if (!gateway.fromString(pSettings->sta_gateway)) {
        USBSerial.printf("❌ Invalid gateway: %s\n", pSettings->sta_gateway);
        return false;
    }

    if (!subnet.fromString(pSettings->sta_subnet)) {
        USBSerial.printf("❌ Invalid subnet: %s\n", pSettings->sta_subnet);
        return false;
    }

    if (!WiFi.config(ip, gateway, subnet)) {
        USBSerial.println("❌ Failed to configure static IP");
        return false;
    }

    USBSerial.println("✅ Static IP configured");
    USBSerial.printf("  IP: %s\n", pSettings->sta_ip);
    USBSerial.printf("  Gateway: %s\n", pSettings->sta_gateway);
    USBSerial.printf("  Subnet: %s\n", pSettings->sta_subnet);

    return true;
}
