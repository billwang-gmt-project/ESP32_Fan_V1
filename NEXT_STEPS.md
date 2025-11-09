# 下一步操作指南

**快速參考**: 您現在需要做什麼

---

## ⚡ 立即執行（5-10 分鐘）

### 1. 編譯並上傳韌體

```bash
# 清除舊建置
pio run -t clean

# 編譯韌體
pio run

# 進入燒錄模式
# 1. 按住 BOOT 按鈕
# 2. 按下 RESET 按鈕
# 3. 放開 BOOT 按鈕

# 上傳韌體
pio run -t upload

# 斷開並重新連接 USB 線（重要！）
```

### 2. 驗證基本功能

```bash
# 開啟序列監視器
pio device monitor

# 執行命令確認版本
INFO

# 預期輸出應包含:
# 韌體版本: v3.0.0
```

---

## 🔧 硬體接線（如需測試馬達控制）

### ⚠️ 重要：GPIO 腳位已變更！

**舊版 (v2.x)**:
- PWM 輸出: GPIO 10
- RPM 輸入: GPIO 11

**新版 (v3.0)**:
- **PWM 輸出: GPIO 17** ← 改到這裡
- **RPM 輸入: GPIO 18** ← 改到這裡

### 最小測試接線

```
ESP32-S3 GPIO 17 ─────── 示波器 CH1 (PWM 輸出)
ESP32-S3 GPIO 12 ─────── 示波器 CH2 (觸發脈衝) [可選]
ESP32-S3 GPIO 18 ─────── 轉速計訊號 [如有]
```

---

## 🧪 基本測試（10-15 分鐘）

在序列監視器中執行以下命令：

```bash
# 1. 切換到 PWM 模式
UART1 MODE PWM

# 2. 設定 PWM 參數
SET PWM_FREQ 1000
SET PWM_DUTY 50

# 3. 檢查狀態
MOTOR STATUS

# 4. 測試高頻（v3.0 新能力！）
SET PWM_FREQ 50000

# 5. 觀察 GPIO 12 脈衝（如有示波器）
# 當您執行上述命令時，GPIO 12 會輸出 10µs 脈衝
```

**預期結果**:
- GPIO 17 輸出 PWM 波形
- GPIO 12 在參數變更時輸出短脈衝
- 命令正常執行無錯誤

---

## 📊 示波器觀察（如有設備）

### 設定

1. **CH1** → GPIO 17 (PWM 輸出)
   - 電壓: 0-3.3V
   - 時基: 根據頻率調整

2. **CH2** → GPIO 12 (觸發脈衝)
   - 觸發模式: 上升沿
   - 觸發電壓: 1.5V

### 測試步驟

```bash
# 設定初始狀態
UART1 MODE PWM
SET PWM_FREQ 1000
SET PWM_DUTY 50

# 改變頻率並觀察
SET PWM_FREQ 10000
# 👀 觀察: GPIO 12 應出現 10µs 脈衝
# 👀 觀察: GPIO 17 的 PWM 波形變化

# 改變占空比並觀察
SET PWM_DUTY 75
# 👀 觀察: GPIO 12 應出現 10µs 脈衝
# 👀 觀察: GPIO 17 的占空比變化
```

**要觀察的重點**:
- ✅ GPIO 12 脈衝寬度約 10µs
- ✅ PWM 參數變更時是否有毛刺
- ✅ 頻率和占空比是否準確
- ✅ 波形是否連續

---

## ❌ 如果遇到問題

### 編譯失敗
```bash
# 完整清除並重建
rm -rf .pio
pio run -t clean
pio run
```

### 上傳失敗
1. 確認 USB 線是資料線（非充電線）
2. 檢查 BOOT 按鈕操作順序
3. 嘗試不同的 USB 埠

### 命令無回應
1. 確認序列監視器的 DTR 已啟用
2. 重新連接 USB 線
3. 執行 `HELP` 查看可用命令

### PWM 無輸出
1. 確認已執行 `UART1 MODE PWM`
2. 檢查接線是否正確（GPIO 17）
3. 確認 `SET PWM_FREQ` 和 `SET PWM_DUTY` 已執行

---

## 📖 詳細文檔參考

**新手入門**:
1. [README.md](README.md) - 專案概述與快速開始
2. [TESTING.md](TESTING.md) - 完整測試指南

**v3.0 變更說明**:
1. [SESSION_SUMMARY.md](SESSION_SUMMARY.md) - 完整變更總結
2. [MOTOR_MERGE_COMPLETED.md](MOTOR_MERGE_COMPLETED.md) - 技術實作細節
3. [GPIO12_PULSE_FEATURE.md](GPIO12_PULSE_FEATURE.md) - GPIO 12 功能說明

**進階開發**:
1. [CLAUDE.md](CLAUDE.md) - 開發者指南（英文）
2. [TODO.md](TODO.md) - 待辦事項清單

---

## 🎯 成功標準

您已成功升級到 v3.0 如果：

- ✅ `INFO` 命令顯示版本 v3.0.0
- ✅ `UART1 MODE PWM` 可成功執行
- ✅ `SET PWM_FREQ 50000` 可成功執行（超過舊版限制）
- ✅ GPIO 17 有 PWM 輸出
- ✅ GPIO 12 在參數變更時有脈衝輸出（如有示波器）

---

## 💡 提示

1. **不要忘記**: 每次上傳韌體後需要斷開並重新連接 USB 線
2. **頻率測試**: 嘗試 `SET PWM_FREQ 50000`，這在舊版是不可能的
3. **保存設定**: 使用 `SAVE` 命令將參數保存到 NVS
4. **WebServer**: 如需 Web 控制，執行 `WIFI <ssid> <password>` 和 `START_WEB`

---

## 🆘 需要幫助？

**常見命令快速參考**:
```bash
HELP              # 顯示所有可用命令
INFO              # 顯示系統資訊和版本
UART1 MODE PWM    # 切換到馬達控制模式
MOTOR STATUS      # 顯示馬達狀態
SET PWM_FREQ <Hz> # 設定 PWM 頻率 (10-500000)
SET PWM_DUTY <%>  # 設定占空比 (0-100)
RPM               # 顯示轉速
SAVE              # 儲存設定
```

**問題回報**:
如遇到問題，請提供：
1. `INFO` 命令的完整輸出
2. 錯誤訊息截圖
3. 示波器波形（如有）

---

**祝測試順利！** 🎉

如有任何問題，請參閱 [TODO.md](TODO.md) 中的詳細測試步驟。
