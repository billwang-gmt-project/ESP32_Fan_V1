#include <Arduino.h>
#include "USB.h"
#include "USBCDC.h"
#include "CustomHID.h"
#include "CommandParser.h"
#include "HIDProtocol.h"
// Motor control is now integrated into UART1Mux
// #include "MotorControl.h"  // DEPRECATED - merged to UART1
// #include "MotorSettings.h"  // DEPRECATED - merged to UART1
#include "StatusLED.h"
#include "WiFiSettings.h"
#include "WiFiManager.h"
#include "WebServer.h"
#include "PeripheralManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <SPIFFS.h>

// USB CDC 實例（由 Arduino 框架自動創建，名為 Serial）
// 注意：當 ARDUINO_USB_CDC_ON_BOOT=1 時，Arduino 框架會自動創建 Serial 對象
// 我們直接使用它，不需要手動創建 USBCDC 實例

// 自訂 HID 實例（64 位元組，無 Report ID）
CustomHID64 HID;

// BLE 相關定義
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_RX "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_TX "beb5483e-36e1-4688-b7f5-ea07361b26a9"

BLEServer* pBLEServer = nullptr;
BLECharacteristic* pTxCharacteristic = nullptr;
BLECharacteristic* pRxCharacteristic = nullptr;
bool bleDeviceConnected = false;
bool bleOldDeviceConnected = false;
// Queue for pending BLE notifications; stores heap-allocated char* messages
QueueHandle_t bleNotifyQueue = nullptr;

// HID 資料結構
typedef struct {
    uint8_t data[64];
    uint16_t len;
    uint8_t report_id;
    uint16_t raw_len;
} HIDDataPacket;

// BLE 命令結構
typedef struct {
    char command[256];  // BLE 命令字串（最大 255 字元 + null terminator）
    uint16_t len;       // 命令長度
} BLECommandPacket;

// FreeRTOS 資源
QueueHandle_t hidDataQueue = nullptr;      // HID 資料佇列
QueueHandle_t bleCommandQueue = nullptr;   // BLE 命令佇列
SemaphoreHandle_t serialMutex = nullptr;   // 保護 Serial 存取
SemaphoreHandle_t bufferMutex = nullptr;   // 保護 hid_out_buffer 存取
SemaphoreHandle_t hidSendMutex = nullptr;  // 保護 HID.send() 存取

// HID 緩衝區（由 mutex 保護）
uint8_t hid_out_buffer[64] = {0};
bool hid_data_ready = false;

// 命令解析器（共用）
CommandParser parser;
CDCResponse* cdc_response = nullptr;
HIDResponse* hid_response = nullptr;
MultiChannelResponse* multi_response = nullptr;  // 多通道回應（同時輸出到 HID 和 CDC）
BLEResponse* ble_response = nullptr;

// HID 命令緩衝區
String hid_command_buffer = "";

// Motor control instances
// Motor control is now integrated into UART1Mux
// MotorControl motorControl;  // DEPRECATED - merged to UART1
// MotorSettingsManager motorSettingsManager;  // DEPRECATED - merged to UART1

// Status LED instance
StatusLED statusLED;

// WiFi and Web Server instances
WiFiSettingsManager wifiSettingsManager;
WiFiManager wifiManager;
WebServerManager webServerManager;

// Peripheral Manager instance
PeripheralManager peripheralManager;

// BLE Server Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        bleDeviceConnected = true;
        Serial.println("[BLE] 客戶端已連接");
        // Flush any queued notifications
        if (bleNotifyQueue) {
            char* msg = nullptr;
            while (xQueueReceive(bleNotifyQueue, &msg, 0) == pdTRUE) {
                if (msg) {
                    if (pTxCharacteristic) {
                        pTxCharacteristic->setValue((uint8_t*)msg, strlen(msg));
                        pTxCharacteristic->notify();
                        delay(10);
                    }
                    free(msg);
                }
            }
        }
    }

    void onDisconnect(BLEServer* pServer) {
        bleDeviceConnected = false;
        Serial.println("[BLE] 客戶端已斷開");
        // 重新開始廣播，允許其他客戶端連接
        delay(500);  // 短暫延遲確保斷開完成
        pServer->startAdvertising();
        Serial.println("[BLE] 重新開始廣播");
    }
};

// BLE RX Characteristic Callbacks (接收來自客戶端的命令)
class MyRxCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0 && rxValue.length() < 256) {
            // 創建命令封包
            BLECommandPacket packet;
            strncpy(packet.command, rxValue.c_str(), sizeof(packet.command) - 1);
            packet.command[sizeof(packet.command) - 1] = '\0';  // 確保 null terminator
            packet.len = rxValue.length();

            // 放入佇列（不阻塞，從回調中調用）
            if (bleCommandQueue) {
                BaseType_t result = xQueueSend(bleCommandQueue, &packet, 0);
                if (result != pdTRUE) {
                    // 佇列已滿，丟棄命令
                    if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(10))) {
                        Serial.println("[BLE] 命令佇列已滿，命令被丟棄");
                        xSemaphoreGive(serialMutex);
                    }
                }
            }
        }
    }
};

// HID 資料接收回調函數（在 ISR 上下文中執行）
void onHIDData(const uint8_t* data, uint16_t len) {
    if (len <= 64) {
        // 準備資料包
        HIDDataPacket packet;
        memcpy(packet.data, data, len);
        packet.len = len;
        packet.report_id = HID.getLastReportId();
        packet.raw_len = HID.getLastRawLen();

        // 從 ISR 發送到佇列（不阻塞）
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(hidDataQueue, &packet, &xHigherPriorityTaskWoken);

        // 如果需要，進行任務切換
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

// HID 處理 Task
void hidTask(void* parameter) {
    HIDDataPacket packet;
    char command_buffer[65] = {0};  // 最多 64 bytes 命令 + null terminator
    uint8_t command_len = 0;
    bool is_0xA1_protocol = false;

    while (true) {
        // 等待 HID 資料
        if (xQueueReceive(hidDataQueue, &packet, portMAX_DELAY)) {

            // 自動偵測並解析命令（支援 0xA1 協定和純文本協定）
            if (HIDProtocol::parseCommand(packet.data, command_buffer, &command_len, &is_0xA1_protocol)) {
                // ========== 這是命令封包 ==========
                if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100))) {
                    const char* protocol_name = is_0xA1_protocol ? "0xA1" : "純文本";
                    Serial.printf("\n[HID CMD %s] %s\n", protocol_name, command_buffer);
                    xSemaphoreGive(serialMutex);
                }

                // 執行命令（根據命令類型路由回應）
                String cmd_str(command_buffer);

                // SCPI 命令 → 只回應到 HID
                // 一般命令 → 只回應到 CDC
                if (CommandParser::isSCPICommand(cmd_str)) {
                    parser.processCommand(cmd_str, hid_response, CMD_SOURCE_HID);
                } else {
                    parser.processCommand(cmd_str, cdc_response, CMD_SOURCE_HID);
                }

                // 顯示提示符
                if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100))) {
                    if (Serial) {
                        Serial.print("> ");
                    }
                    xSemaphoreGive(serialMutex);
                }

            } else {
                // ========== 這是原始資料（非命令）==========
                // 更新共享緩衝區（加鎖）
                if (xSemaphoreTake(bufferMutex, portMAX_DELAY)) {
                    memcpy(hid_out_buffer, packet.data, packet.len);
                    hid_data_ready = true;
                    xSemaphoreGive(bufferMutex);
                }

                // 顯示除錯資訊（加鎖保護 Serial）
                if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100))) {
                    Serial.printf("\n[DEBUG] HID OUT (原始資料): %d 位元組\n", packet.len);

                    // 顯示前 16 bytes
                    Serial.print("前16: ");
                    for (uint16_t i = 0; i < packet.len && i < 16; i++) {
                        Serial.printf("%02X ", packet.data[i]);
                    }
                    Serial.println();

                    // 顯示最後 16 bytes
                    if (packet.len > 16) {
                        uint16_t start = packet.len - 16;
                        Serial.print("後16: ");
                        for (uint16_t i = start; i < packet.len; i++) {
                            Serial.printf("%02X ", packet.data[i]);
                        }
                        Serial.println();
                    }
                    Serial.print("> ");

                    xSemaphoreGive(serialMutex);
                }
            }
        }
    }
}

// CDC 處理 Task
void cdcTask(void* parameter) {
    String cdc_command_buffer = "";

    while (true) {
        // 檢查是否有可用資料（使用 mutex 保護）
        int available = 0;
        if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100))) {
            available = Serial.available();
            xSemaphoreGive(serialMutex);
        }

        // 處理 CDC 輸入
        while (available > 0) {
            char c = 0;

            // 讀取單個字元（使用 mutex 保護）
            if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100))) {
                if (Serial.available()) {
                    c = Serial.read();
                    available = Serial.available();  // 更新剩餘可用資料數量
                } else {
                    available = 0;  // 沒有資料了
                }
                xSemaphoreGive(serialMutex);
            } else {
                break;  // 無法獲取 mutex，跳出迴圈
            }

            // 處理接收到的字元（在 mutex 外部，不影響其他 task）
            if (c == '\n' || c == '\r') {
                // 收到換行符，處理完整命令
                if (cdc_command_buffer.length() > 0) {
                    // 取得 mutex 保護 Serial 輸出
                    if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(1000))) {
                        // 處理命令（CDC 命令只輸出到 CDC）
                        parser.processCommand(cdc_command_buffer, cdc_response, CMD_SOURCE_CDC);
                        cdc_command_buffer = "";  // 清空緩衝區

                        // 顯示提示符
                        Serial.print("> ");
                        xSemaphoreGive(serialMutex);
                    }
                }
            } else if (c == '\b' || c == 127) {
                // 退格鍵
                if (cdc_command_buffer.length() > 0) {
                    cdc_command_buffer.remove(cdc_command_buffer.length() - 1);
                }
            } else if (c >= 32 && c < 127) {
                // 可列印字元
                cdc_command_buffer += c;
            }
        }

        // 短暫延遲避免佔用 CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// BLE 處理 Task
void bleTask(void* parameter) {
    BLECommandPacket packet;

    while (true) {
        // 從佇列接收命令（阻塞等待）
        if (xQueueReceive(bleCommandQueue, &packet, portMAX_DELAY) == pdTRUE) {
            String command = String(packet.command);
            command.trim();

            // 調試輸出（保護 Serial）
            if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100))) {
                Serial.printf("\n[BLE CMD] %s\n", command.c_str());
                xSemaphoreGive(serialMutex);
            }

            // 處理 BLE 命令（根據命令類型路由回應）
            // 重要：在任務上下文中調用，不在 BLE 回調中！
            // SCPI 命令 → 只回應到 BLE
            // 一般命令 → 只回應到 CDC
            if (CommandParser::isSCPICommand(command)) {
                parser.processCommand(command, ble_response, CMD_SOURCE_BLE);
            } else {
                parser.processCommand(command, cdc_response, CMD_SOURCE_BLE);
            }

            // 短暫延遲確保 BLE 通知發送完成
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

// WiFi 處理 Task
void wifiTask(void* parameter) {
    TickType_t lastWiFiUpdate = 0;
    TickType_t lastWebUpdate = 0;

    while (true) {
        TickType_t now = xTaskGetTickCount();

        // Update WiFi status (check connection, handle reconnection)
        if (now - lastWiFiUpdate >= pdMS_TO_TICKS(1000)) {  // Every 1 second
            wifiManager.update();
            lastWiFiUpdate = now;
        }

        // Update web server (WebSocket broadcasts, cleanup)
        if (now - lastWebUpdate >= pdMS_TO_TICKS(200)) {  // Every 200ms (5 Hz)
            if (webServerManager.isRunning()) {
                webServerManager.update();
            }
            lastWebUpdate = now;
        }

        // Yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(50));  // 50ms loop rate
    }
}

// Peripheral 處理 Task
void peripheralTask(void* parameter) {
    while (true) {
        // Update all peripherals
        // - User key debouncing and event detection
        // - UART1 RPM measurement (if in PWM/RPM mode)
        // - Motor control via keys
        peripheralManager.update();

        // Yield to other tasks (20ms loop rate for responsive key handling)
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// Peripheral 處理 Task (migrated from motorTask)
void motorTask(void* parameter) {
    TickType_t lastRPMUpdate = 0;
    TickType_t lastLEDUpdate = 0;

    while (true) {
        TickType_t now = xTaskGetTickCount();

        // Update UART1 RPM reading if in PWM/RPM mode (every 50ms)
        if (now - lastRPMUpdate >= pdMS_TO_TICKS(50)) {
            peripheralManager.getUART1().updateRPMFrequency();
            lastRPMUpdate = now;
        }

        // Update LED based on system state every 200ms
        if (now - lastLEDUpdate >= pdMS_TO_TICKS(200)) {
            auto& uart1 = peripheralManager.getUART1();

            // Priority 1: Web server not ready - blink yellow as warning
            if (!webServerManager.isRunning()) {
                statusLED.blinkYellow(500);
            }
            // Priority 2: BLE connected - purple
            else if (bleDeviceConnected) {
                statusLED.setPurple();
            }
            // Priority 3: UART1 PWM active - blue (motor running)
            else if (uart1.getMode() == UART1Mux::MODE_PWM_RPM &&
                     uart1.isPWMEnabled() &&
                     uart1.getPWMDuty() > 0.1) {
                statusLED.setBlue();
            }
            // Priority 4: Motor idle - green
            else {
                statusLED.setGreen();
            }
            lastLEDUpdate = now;
        }

        // Yield to other tasks (50ms loop rate)
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void setup() {
    // ========== 步驟 1: 初始化 USB ==========
    Serial.begin();
    HID.begin();
    HID.onData(onHIDData);
    USB.begin();

    // ========== 步驟 1.5: 初始化狀態 LED ==========
    // Initialize status LED (default brightness: 25)
    if (!statusLED.begin(48, 25)) {
        Serial.println("⚠️ Status LED initialization failed!");
    } else {
        // Show yellow blinking during initialization
        statusLED.blinkYellow(200);
    }

    // ========== 步驟 1.6: 初始化週邊管理器 ==========
    // Motor control is now integrated into UART1Mux (no separate motor control)
    Serial.println("");
    if (!peripheralManager.begin()) {  // Motor control now in UART1
        Serial.println("❌ Peripheral manager initialization failed!");
        // Non-critical - system can continue without peripherals
    } else {
        Serial.println("✅ Peripheral manager initialized successfully");

        // Initialize peripheral settings
        if (peripheralManager.beginSettings()) {
            Serial.println("✅ Peripheral settings manager initialized");

            // Load settings from NVS
            if (peripheralManager.loadSettings()) {
                Serial.println("✅ Peripheral settings loaded from NVS");

                // Apply settings to all peripherals
                if (peripheralManager.applySettings()) {
                    Serial.println("✅ Peripheral settings applied");
                } else {
                    Serial.println("⚠️ Some peripheral settings may not have been applied");
                }
            } else {
                Serial.println("ℹ️ Using default peripheral settings");
            }
        } else {
            Serial.println("❌ Peripheral settings manager initialization failed");
        }

        // Force UART1 to PWM/RPM mode at startup (non-persistent default)
        Serial.println("");
        Serial.println("🔧 Setting UART1 to default PWM/RPM mode...");
        if (peripheralManager.getUART1().setModePWM_RPM()) {
            Serial.println("✅ UART1 set to PWM/RPM mode (default)");
        } else {
            Serial.println("⚠️ Failed to set UART1 to PWM/RPM mode");
        }
    }

    // ========== 步驟 2: 創建 FreeRTOS 資源（必須在 BLE 初始化之前！）==========
    hidDataQueue = xQueueCreate(10, sizeof(HIDDataPacket));
    bleCommandQueue = xQueueCreate(10, sizeof(BLECommandPacket));  // BLE 命令佇列
    serialMutex = xSemaphoreCreateMutex();
    bufferMutex = xSemaphoreCreateMutex();
    hidSendMutex = xSemaphoreCreateMutex();
    bleNotifyQueue = xQueueCreate(32, sizeof(char*));

    // 檢查資源創建是否成功
    if (!hidDataQueue || !bleCommandQueue || !serialMutex || !bufferMutex || !hidSendMutex || !bleNotifyQueue) {
        Serial.println("❌ CRITICAL ERROR: FreeRTOS resource creation failed!");
        // Critical error - flash red LED fast and halt
        statusLED.blinkRed(100);
        statusLED.update();  // Update once to show the LED state
        // 如果資源創建失敗，進入無限迴圈（需要重啟）
        while (true) {
            statusLED.update();  // Keep LED blinking
            delay(10);
        }
    }

    // ========== 步驟 3: 創建回應物件 ==========
    cdc_response = new CDCResponse(Serial);
    hid_response = new HIDResponse(&HID);
    multi_response = new MultiChannelResponse(cdc_response, hid_response);

    // ========== 步驟 4: 等待 USB 連接（在 BLE 初始化之前）==========
    unsigned long start = millis();
    while (!Serial && (millis() - start < 5000)) {
        statusLED.update();  // Update LED during wait to show blinking
        delay(100);
    }

    // ========== 步驟 5: 顯示歡迎訊息 ==========
    Serial.println("\n=================================");
    Serial.println("ESP32-S3 馬達控制系統");
    Serial.println("=================================");
    Serial.println("系統功能:");
    Serial.println("  ✅ USB CDC 序列埠控制台");
    Serial.println("  ✅ USB HID 自訂協定 (64 bytes)");
    Serial.println("  ✅ BLE GATT 無線介面");
    Serial.println("  ✅ WiFi Web 伺服器（AP/STA 模式）");
    Serial.println("  ✅ WebSocket 即時 RPM 監控");
    Serial.println("  ✅ REST API 馬達控制");
    Serial.println("  ✅ PWM 馬達控制 (MCPWM)");
    Serial.println("  ✅ 轉速計 RPM 量測");
    Serial.println("  ✅ FreeRTOS 多工架構");
    Serial.println("");
    Serial.println("硬體配置:");
    Serial.println("  GPIO 10: PWM 輸出");
    Serial.println("  GPIO 11: 轉速計輸入");
    Serial.println("  GPIO 12: 脈衝輸出");
    Serial.println("");
    Serial.printf("初始設定:\n");
    Serial.printf("  PWM 頻率: %u Hz\n", peripheralManager.getUART1().getPWMFrequency());
    Serial.printf("  PWM 占空比: %.1f%%\n", peripheralManager.getUART1().getPWMDuty());
    Serial.printf("  極對數: %d\n", peripheralManager.getUART1().getPolePairs());
    Serial.println("");
    Serial.println("輸入 'HELP' 查看所有命令");
    Serial.println("=================================");

    // ========== 步驟 5.5: 初始化 SPIFFS 檔案系統 ==========
    Serial.println("");
    Serial.println("=== 初始化 SPIFFS 檔案系統 ===");

    if (!SPIFFS.begin(true)) {  // true = format if mount fails
        Serial.println("❌ SPIFFS mount failed!");
        Serial.println("  Web 介面將使用內建 HTML（備用模式）");
    } else {
        Serial.println("✅ SPIFFS mounted successfully");

        // List files in SPIFFS for debugging
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        if (file) {
            Serial.println("📁 SPIFFS files:");
            while (file) {
                Serial.printf("  - %s (%d bytes)\n", file.name(), file.size());
                file = root.openNextFile();
            }
        } else {
            Serial.println("  ⚠️ No files found in SPIFFS");
            Serial.println("  請使用 'pio run --target uploadfs' 上傳檔案");
        }
    }

    Serial.println("=================================");

    // ========== 步驟 6: 初始化 WiFi 和 Web 伺服器 ==========
    Serial.println("");
    Serial.println("=== 初始化 WiFi 和 Web 伺服器 ===");

    // Initialize WiFi settings
    if (!wifiSettingsManager.begin()) {
        Serial.println("⚠️ WiFi settings initialization failed, using defaults");
    }

    // Load WiFi settings from NVS
    wifiSettingsManager.load();
    const WiFiSettings& wifiSettings = wifiSettingsManager.get();

    // Initialize WiFi manager
    if (!wifiManager.begin(const_cast<WiFiSettings*>(&wifiSettings))) {
        Serial.println("❌ WiFi manager initialization failed!");
    } else {
        Serial.println("✅ WiFi manager initialized");
    }

    // Initialize web server (motor control now in UART1)
    if (!webServerManager.begin(
        const_cast<WiFiSettings*>(&wifiSettings),
        &wifiManager,
        &statusLED,
        &peripheralManager,
        &wifiSettingsManager
    )) {
        Serial.println("❌ Web server initialization failed!");
    } else {
        Serial.println("✅ Web server initialized");
    }

    // Start WiFi if configured
    if (wifiSettings.mode != WiFiMode::OFF) {
        Serial.printf("🔧 啟動 WiFi 模式: ");
        switch (wifiSettings.mode) {
            case WiFiMode::AP:
                Serial.println("Access Point");
                break;
            case WiFiMode::STA:
                Serial.println("Station");
                break;
            case WiFiMode::AP_STA:
                Serial.println("AP + Station");
                break;
            default:
                Serial.println("Unknown");
                break;
        }

        statusLED.update();  // Update LED before WiFi start
        if (wifiManager.start()) {
            Serial.println("✅ WiFi started successfully");
            statusLED.update();  // Update LED after WiFi start

            // Start web server if WiFi is connected
            if (wifiManager.isConnected()) {
                statusLED.update();  // Update LED before web server start
                if (webServerManager.start()) {
                    Serial.println("✅ Web server started successfully");
                    Serial.println("");
                    Serial.println("🌐 Web 介面資訊:");
                    Serial.printf("  URL: http://%s/\n", wifiManager.getIPAddress().c_str());
                    Serial.printf("  WebSocket: ws://%s/ws\n", wifiManager.getIPAddress().c_str());
                    Serial.println("  可透過網頁控制馬達並即時查看 RPM");
                } else {
                    Serial.println("⚠️ Web server failed to start");
                }
            }
        } else {
            Serial.println("⚠️ WiFi failed to start");
            Serial.println("  使用 'WIFI START' 命令手動啟動");
        }
    } else {
        Serial.println("ℹ️ WiFi 模式: OFF (未啟動)");
        Serial.println("  使用 'WIFI START' 命令啟動 WiFi");
    }

    Serial.println("=================================");

    // ========== 步驟 7: 初始化 BLE（現在 mutex 已準備好）==========
    Serial.println("[INFO] 正在初始化 BLE...");
    statusLED.update();  // Update LED during initialization

    BLEDevice::init("ESP32-S3 Motor Control");
    pBLEServer = BLEDevice::createServer();
    pBLEServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pBLEServer->createService(SERVICE_UUID);

    // TX Characteristic (用於發送資料到客戶端)
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic->addDescriptor(new BLE2902());

    // RX Characteristic (用於接收來自客戶端的資料)
    pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE
    );
    pRxCharacteristic->setCallbacks(new MyRxCallbacks());

    pService->start();
    statusLED.update();  // Update LED after service start

    // 開始廣播
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();
    statusLED.update();  // Update LED after advertising start

    // 創建 BLE 回應物件
    ble_response = new BLEResponse(pTxCharacteristic);

    Serial.println("[INFO] BLE 初始化完成");
    Serial.println("\nBluetooth 資訊:");
    Serial.println("  BLE 裝置名稱: ESP32_S3_Console");
    Serial.println("=================================");
    Serial.print("\n> ");

    // 創建 FreeRTOS Tasks
    statusLED.update();  // Update LED before creating tasks
    xTaskCreatePinnedToCore(
        hidTask,           // Task 函數
        "HID_Task",        // Task 名稱
        4096,              // Stack 大小
        NULL,              // 參數
        2,                 // 優先權（較高）
        NULL,              // Task handle
        1                  // Core 1
    );

    xTaskCreatePinnedToCore(
        cdcTask,           // Task 函數
        "CDC_Task",        // Task 名稱
        4096,              // Stack 大小
        NULL,              // 參數
        1,                 // 優先權（較低）
        NULL,              // Task handle
        1                  // Core 1
    );

    xTaskCreatePinnedToCore(
        bleTask,           // Task 函數
        "BLE_Task",        // Task 名稱
        4096,              // Stack 大小
        NULL,              // 參數
        1,                 // 優先權（與 CDC 相同）
        NULL,              // Task handle
        1                  // Core 1
    );

    xTaskCreatePinnedToCore(
        motorTask,         // Task 函數
        "Motor_Task",      // Task 名稱
        4096,              // Stack 大小
        NULL,              // 參數
        1,                 // 優先權（與 CDC 相同）
        NULL,              // Task handle
        1                  // Core 1
    );

    xTaskCreatePinnedToCore(
        wifiTask,          // Task 函數
        "WiFi_Task",       // Task 名稱
        8192,              // Stack 大小（較大，因為需要處理 WiFi 和 Web Server）
        NULL,              // 參數
        1,                 // 優先權（與 CDC 相同）
        NULL,              // Task handle
        1                  // Core 1
    );

    xTaskCreatePinnedToCore(
        peripheralTask,    // Task 函數
        "Peripheral_Task", // Task 名稱
        4096,              // Stack 大小
        NULL,              // 參數
        1,                 // 優先權（與 CDC 相同）
        NULL,              // Task handle
        1                  // Core 1
    );

    Serial.println("[INFO] FreeRTOS Tasks 已啟動");
    Serial.println("[INFO] - HID Task (優先權 2)");
    Serial.println("[INFO] - CDC Task (優先權 1)");
    Serial.println("[INFO] - BLE Task (優先權 1)");
    Serial.println("[INFO] - Motor Task (優先權 1)");
    Serial.println("[INFO] - WiFi Task (優先權 1)");
    Serial.println("[INFO] - Peripheral Task (優先權 1)");

    // LED state will be managed by motorTask based on actual system status
    // Don't set it here to avoid confusion
    Serial.println("✅ System initialization complete");
    Serial.println("ℹ️ LED status will be updated by Motor Task based on system state");
}

void loop() {
    // Update status LED (for blink timing)
    statusLED.update();

    // FreeRTOS Tasks 處理所有工作
    // loop() 保持空閒，讓 IDLE task 運行
    vTaskDelay(pdMS_TO_TICKS(50));  // Reduced to 50ms for smoother LED blinking
}
