# 待辦事項 (TODO List)

**專案**: ESP32-S3 USB 複合裝置
**當前版本**: v3.0.0
**最後更新**: 2025-11-09

---

## 🔴 高優先級 - 必須完成

### 1. 編譯與基本測試 ⚠️
**狀態**: 未測試
**負責**: 開發者
**預計時間**: 30 分鐘

**任務**:
- [ ] 執行 `pio run -t clean && pio run` 確認編譯成功
- [ ] 上傳韌體到 ESP32-S3
- [ ] 驗證 USB CDC 連接正常
- [ ] 執行 `INFO` 命令確認版本為 v3.0.0

**預期結果**:
```
韌體版本: v3.0.0 (motor-merge-to-uart1)
編譯時間: 2025-11-09
```

---

### 2. UART1 馬達控制功能測試 🎯
**狀態**: 未測試
**負責**: 開發者
**預計時間**: 1-2 小時

#### 2.1 基本 PWM 功能
- [ ] `UART1 MODE PWM` - 切換到 PWM/RPM 模式
- [ ] `SET PWM_FREQ 1000` - 設定 1 kHz
- [ ] `SET PWM_DUTY 50` - 設定 50% 占空比
- [ ] 使用示波器確認 GPIO 17 輸出正確的 PWM 波形

#### 2.2 頻率範圍測試（驗證 MCPWM 優勢）
- [ ] 低頻測試: `SET PWM_FREQ 10` (10 Hz)
- [ ] 中低頻測試: `SET PWM_FREQ 1000` (1 kHz)
- [ ] **關鍵測試**: `SET PWM_FREQ 10000` (10 kHz) - 舊版 LEDC 限制處
- [ ] 高頻測試: `SET PWM_FREQ 50000` (50 kHz)
- [ ] 極高頻測試: `SET PWM_FREQ 100000` (100 kHz)
- [ ] 最大頻率測試: `SET PWM_FREQ 500000` (500 kHz)

#### 2.3 占空比測試
- [ ] 0% 占空比: `SET PWM_DUTY 0`
- [ ] 25% 占空比: `SET PWM_DUTY 25`
- [ ] 50% 占空比: `SET PWM_DUTY 50`
- [ ] 75% 占空比: `SET PWM_DUTY 75`
- [ ] 100% 占空比: `SET PWM_DUTY 100`

#### 2.4 RPM 測量功能
- [ ] 連接轉速計訊號到 GPIO 18
- [ ] 執行 `RPM` 命令
- [ ] 驗證測量的頻率準確
- [ ] `SET POLE_PAIRS 2` 設定極對數
- [ ] 確認 RPM 計算正確: RPM = (freq × 60) / pole_pairs

#### 2.5 馬達控制命令（向下兼容性）
- [ ] `MOTOR STATUS` - 顯示完整狀態
- [ ] `MOTOR STOP` - 緊急停止
- [ ] `SAVE` - 儲存設定到 NVS
- [ ] `LOAD` - 從 NVS 載入設定
- [ ] `RESET` - 重設為預設值

#### 2.6 UART1 模式切換
- [ ] PWM/RPM → UART: `UART1 MODE UART`
- [ ] UART → PWM/RPM: `UART1 MODE PWM`
- [ ] 驗證 GPIO 17, 18 功能正確切換

---

### 3. GPIO 12 脈衝功能測試 🔍
**狀態**: 未測試
**負責**: 開發者
**預計時間**: 30-60 分鐘

#### 3.1 示波器設定
- [ ] CH1 連接到 GPIO 17 (PWM 輸出)
- [ ] CH2 連接到 GPIO 12 (觸發脈衝)
- [ ] 設定觸發: CH2 上升沿
- [ ] 時基: 10-100µs/div

#### 3.2 頻率變更觀察
- [ ] 設定初始頻率: `SET PWM_FREQ 1000`
- [ ] 改變頻率: `SET PWM_FREQ 10000`
- [ ] 確認 GPIO 12 輸出 10µs 脈衝
- [ ] 觀察 GPIO 17 是否有毛刺或不連續

#### 3.3 占空比變更觀察
- [ ] 設定初始占空比: `SET PWM_DUTY 50`
- [ ] 改變占空比: `SET PWM_DUTY 75`
- [ ] 確認 GPIO 12 輸出 10µs 脈衝
- [ ] 觀察 GPIO 17 是否有毛刺或不連續

#### 3.4 記錄觀察結果
- [ ] 拍攝示波器波形照片
- [ ] 記錄是否觀察到毛刺
- [ ] 比較不同頻率下的行為
- [ ] 比較 MCPWM vs LEDC (如有舊版韌體)

---

## 🟡 中優先級 - 重要但非緊急

### 4. WebServer 整合測試 🌐
**狀態**: 部分完成（需手動測試）
**負責**: 開發者
**預計時間**: 1-2 小時

#### 4.1 WiFi 連接
- [ ] `WIFI <ssid> <password>` 連接到 WiFi
- [ ] 驗證 IP 位址顯示
- [ ] `START_WEB` 啟動 Web 伺服器

#### 4.2 Web API 端點測試
- [ ] `GET /api/status` - 確認回傳 UART1 狀態
- [ ] `POST /api/pwm` - 透過 API 設定 PWM
- [ ] `POST /api/save` - 透過 API 儲存設定
- [ ] `GET /api/rpm` - 確認回傳 UART1 RPM

#### 4.3 Web 控制面板
- [ ] 開啟瀏覽器訪問 Web 介面
- [ ] 測試頻率滑桿控制
- [ ] 測試占空比滑桿控制
- [ ] 測試即時 RPM 顯示

**已知問題**:
- ⚠️ WebServer 可能需要進一步修正（見 SESSION_SUMMARY.md）

---

### 5. 設定持久化測試 💾
**狀態**: 未測試
**負責**: 開發者
**預計時間**: 30 分鐘

- [ ] 設定 PWM 參數並儲存: `SET PWM_FREQ 5000`, `SET PWM_DUTY 60`, `SAVE`
- [ ] 重啟 ESP32-S3 (斷電重連)
- [ ] 執行 `LOAD` 載入設定
- [ ] 確認參數正確恢復
- [ ] 測試 `RESET` 恢復預設值

---

### 6. 週邊控制測試 🔧
**狀態**: 可能未受影響，但需驗證
**負責**: 開發者
**預計時間**: 1 小時

- [ ] UART2 功能: `UART2 CONFIG 115200`, `UART2 WRITE test`
- [ ] 蜂鳴器: `BUZZER 2000 50 ON`
- [ ] LED PWM: `LED_PWM 1000 50 ON`
- [ ] 繼電器: `RELAY ON`, `RELAY OFF`
- [ ] GPIO: `GPIO HIGH`, `GPIO LOW`
- [ ] 使用者按鍵: 測試按鍵功能

---

## 🟢 低優先級 - 可選或長期

### 7. 效能與穩定性測試 📊
**狀態**: 未開始
**負責**: 開發者
**預計時間**: 2-4 小時

#### 7.1 長時間運行測試
- [ ] 連續運行 24 小時
- [ ] 監控記憶體洩漏
- [ ] 檢查系統穩定性

#### 7.2 頻率切換壓力測試
- [ ] 快速連續改變頻率 100 次
- [ ] 監控是否有異常或當機
- [ ] 記錄每次切換的響應時間

#### 7.3 MCPWM vs LEDC 效能比較
- [ ] 測量頻率準確度
- [ ] 測量占空比準確度
- [ ] 測量參數變更響應時間
- [ ] 比較 CPU 負載

---

### 8. 文檔完善 📝
**狀態**: 部分完成
**負責**: 開發者
**預計時間**: 2-3 小時

#### 8.1 已完成
- [x] SESSION_SUMMARY.md - 會話總結
- [x] MOTOR_MERGE_COMPLETED.md - 技術文檔
- [x] GPIO12_PULSE_FEATURE.md - GPIO 12 功能文檔
- [x] README.md v3.0 章節 - 使用者指南更新
- [x] TODO.md - 本文檔

#### 8.2 待完成
- [ ] 更新 CLAUDE.md 的馬達控制章節
- [ ] 更新 TESTING.md 添加 v3.0 測試案例
- [ ] 更新 PROTOCOL.md (如需要)
- [ ] 創建 CHANGELOG.md 記錄版本歷史
- [ ] 拍攝示波器照片並添加到文檔
- [ ] 創建 v3.0 測試報告

---

### 9. 代碼品質改進 🔨
**狀態**: 未開始
**負責**: 開發者
**預計時間**: 3-5 小時

- [ ] 清理 `src/WebServer.cpp.backup` 備份檔案
- [ ] 移除 `*.deprecated` 檔案（或移到 archive 資料夾）
- [ ] 添加單元測試 (如適用)
- [ ] 代碼覆蓋率分析
- [ ] 靜態分析工具掃描

---

### 10. 進階功能開發 🚀
**狀態**: 構思階段
**負責**: 開發者
**預計時間**: TBD

#### 10.1 可能的新功能
- [ ] PWM 頻率/占空比漸變 (MCPWM 版本)
- [ ] RPM 濾波器 (移動平均)
- [ ] 自動調速控制 (PID)
- [ ] 故障檢測與報警
- [ ] 資料記錄與匯出

#### 10.2 效能優化
- [ ] 優化 MCPWM 配置參數
- [ ] 減少任務切換開銷
- [ ] 優化 WebServer 回應速度

---

## 📋 測試檢查清單總結

### 必須通過的測試 (Release Blocker)
1. ✅ 編譯成功
2. ⬜ USB CDC 連接正常
3. ⬜ UART1 PWM 輸出正確 (GPIO 17)
4. ⬜ UART1 RPM 測量正常 (GPIO 18)
5. ⬜ 馬達控制命令向下兼容
6. ⬜ 頻率範圍 10 Hz - 500 kHz 可用
7. ⬜ NVS 設定持久化運作
8. ⬜ GPIO 12 脈衝輸出正常

### 建議通過的測試 (Nice to Have)
1. ⬜ WebServer API 完全運作
2. ⬜ 所有週邊控制正常
3. ⬜ 長時間穩定性測試
4. ⬜ 完整文檔更新

---

## 🐛 已知問題與限制

### 已知問題
1. **WebServer 整合**: 可能需要進一步測試和修正（見 SESSION_SUMMARY.md）
2. **進階功能移除**: Ramping, Filtering, Watchdog 功能不再可用（被視為非關鍵）

### 限制
1. **MCPWM 資源**: MCPWM_UNIT_0 和 MCPWM_UNIT_1 已被 UART1 使用
2. **GPIO 腳位**: GPIO 17, 18 在 PWM/RPM 模式下無法用於 UART

---

## 📞 需要協助？

**技術支援**:
- 參閱 [SESSION_SUMMARY.md](SESSION_SUMMARY.md) 了解完整變更
- 參閱 [MOTOR_MERGE_COMPLETED.md](MOTOR_MERGE_COMPLETED.md) 了解技術細節
- 參閱 [GPIO12_PULSE_FEATURE.md](GPIO12_PULSE_FEATURE.md) 了解偵錯功能

**回報問題**:
- 請記錄詳細的錯誤訊息
- 提供示波器波形截圖 (如適用)
- 註明韌體版本 (執行 `INFO` 命令)

---

**最後更新**: 2025-11-09
**文檔版本**: 1.0
**對應韌體**: v3.0.0
