#ifndef WIFI_SETTINGS_H
#define WIFI_SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>

/**
 * @brief WiFi connection modes
 */
enum class WiFiMode {
    OFF,        // WiFi disabled
    AP,         // Access Point mode
    STA,        // Station mode (connect to existing network)
    AP_STA      // Both AP and Station (fallback mode)
};

/**
 * @brief WiFi settings structure
 *
 * Stored in NVS under "wifi" namespace.
 */
struct WiFiSettings {
    // WiFi Mode
    WiFiMode mode;

    // Access Point settings
    char ap_ssid[32];
    char ap_password[64];
    uint8_t ap_channel;

    // Station settings
    char sta_ssid[32];
    char sta_password[64];
    bool sta_dhcp;
    char sta_ip[16];
    char sta_gateway[16];
    char sta_subnet[16];

    // Web server settings
    uint16_t web_port;
    bool web_auth_enabled;
    char web_username[32];
    char web_password[64];
};

/**
 * @brief Default WiFi settings
 */
namespace WiFiDefaults {
    extern const WiFiMode MODE;
    extern const char* AP_SSID;
    extern const char* AP_PASSWORD;
    extern const uint8_t AP_CHANNEL;
    extern const char* STA_SSID;
    extern const char* STA_PASSWORD;
    extern const bool STA_DHCP;
    extern const char* STA_IP;
    extern const char* STA_GATEWAY;
    extern const char* STA_SUBNET;
    extern const uint16_t WEB_PORT;
    extern const bool WEB_AUTH_ENABLED;
    extern const char* WEB_USERNAME;
    extern const char* WEB_PASSWORD;
}

/**
 * @brief WiFi settings manager
 *
 * Manages WiFi configuration persistence using NVS.
 */
class WiFiSettingsManager {
public:
    /**
     * @brief Constructor
     */
    WiFiSettingsManager();

    /**
     * @brief Initialize settings manager
     * @return true if successful
     */
    bool begin();

    /**
     * @brief Load settings from NVS
     * @return true if settings loaded, false if using defaults
     */
    bool load();

    /**
     * @brief Save current settings to NVS
     * @return true if successful
     */
    bool save();

    /**
     * @brief Reset to default settings
     */
    void reset();

    /**
     * @brief Get current settings (read-only)
     * @return const reference to settings
     */
    const WiFiSettings& get() const;

    /**
     * @brief Get current settings (read-write)
     * @return reference to settings
     */
    WiFiSettings& get();

private:
    WiFiSettings settings;
    Preferences prefs;
    static const char* NVS_NAMESPACE;
};

#endif // WIFI_SETTINGS_H
