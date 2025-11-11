# PWM 頻率輸出錯誤修復備忘錄

**日期**: 2025-01-11
**專案**: ESP32_Fan_V1
**問題**: UART1 PWM 頻率輸出錯誤
**狀態**: 已修復，待測試驗證

---

## 問題描述

### 症狀
使用 `UART1 PWM <freq> <duty>` 命令設定 PWM 頻率時，實際輸出頻率與設定值不符：
- 設定 1000 Hz → 實際輸出錯誤
- 設定 10000 Hz → 實際輸出錯誤
- 設定 100000 Hz → 實際輸出正確（巧合）

### Debug 輸出觀察
```
[DEBUG] handleUART1PWM: Calling setPWMFrequencyAndDuty(1000 Hz, 50.0%)
[REG] BEFORE: cfg0=0x001F3F09, period=8000
[REG] AFTER:  cfg0=0x001F3F09  ← 寫入失敗！值沒有改變
```

Register 寫入前後值相同，表示寫入操作沒有生效。

---

## 根本原因分析

### 問題 1: 錯誤的 Register Mask

**位置**: `src/UART1Mux.cpp:379`
**原始程式碼**:
```cpp
uint32_t cfg0_new = (cfg0_before & 0xFF0000FF)  // ❌ 錯誤！
                  | (((new_period - 1) & 0xFFFF) << 8);
```

**問題**:
- `0xFF0000FF` mask 會清除 bits[23:16]（period 的高 8 位元）
- 只保留 bits[31:24] 和 bits[7:0]
- 結果：period 只有低 8 位元 (bits[15:8]) 被正確寫入

**影響範圍**:
| 頻率 | 期望 Period | 寫入值 | 結果 |
|------|------------|--------|------|
| 1000 Hz | 8000 (0x1F40) | 0x0040 = 64 | ❌ 錯誤 |
| 10000 Hz | 800 (0x0320) | 0x0020 = 32 | ❌ 錯誤 |
| 100000 Hz | 80 (0x0050) | 0x0050 = 80 | ✓ 正確（巧合，高位元本來就是 0）|

**正確的 Mask**:
```cpp
uint32_t cfg0_new = (cfg0_before & 0xFFFF00FF)  // ✅ 正確
                  | (((new_period - 1) & 0xFFFF) << 8);
```

### 問題 2: 缺少 Critical Section 保護

**位置**: `src/UART1Mux.cpp:333-405` (`setPWMFrequencyAndDuty` 函數)

**問題**:
- 直接寫入 MCPWM register 沒有使用 `taskENTER_CRITICAL`
- 寫入操作可能被 FreeRTOS 任務切換或中斷打斷
- 導致 register 寫入不完整或被覆蓋

**對比**: `updatePWMRegistersDirectly()` 函數有正確實作
```cpp
void UART1Mux::updatePWMRegistersDirectly(uint32_t period, float duty) {
    taskENTER_CRITICAL(&mux);  // ✅ 正確保護

    // Update period register...
    MCPWM1.timer[0].timer_cfg0.val = cfg0_new;

    taskEXIT_CRITICAL(&mux);

    // Update duty (ESP-IDF API 已有內部保護)
    mcpwm_set_duty(...);
}
```

---

## 解決方案

### 修改內容

**檔案**: `src/UART1Mux.cpp`
**函數**: `bool UART1Mux::setPWMFrequencyAndDuty(uint32_t frequency, float duty)`
**行數**: 333-405

**修改前** (錯誤實作):
```cpp
// 直接寫入 register，沒有保護
uint32_t cfg0_before = MCPWM1.timer[0].timer_cfg0.val;
uint32_t cfg0_new = (cfg0_before & 0xFF0000FF)  // ❌ 錯誤 mask
                  | (((new_period - 1) & 0xFFFF) << 8);
MCPWM1.timer[0].timer_cfg0.val = cfg0_new;
```

**修改後** (正確實作):
```cpp
// 使用統一的 shadow register 更新函數
updatePWMRegistersDirectly(new_period, duty);
```

### 修改優勢

1. **使用現有的正確實作**
   - `updatePWMRegistersDirectly()` 已經過驗證
   - 包含 critical section 保護
   - 使用正確的 mask (`0xFFFF00FF`)

2. **Shadow Register Mode**
   - Period 更新設定 bit[24]=1 (TEZ 同步)
   - Duty 更新使用 `mcpwm_set_duty()` (內建 TEZ 同步)
   - 兩者在同一個 TEZ 事件同步生效 → 完全無 glitch

3. **原子性保證**
   - Critical section 確保 period 寫入不被中斷
   - Period 和 duty 同步更新，避免不一致狀態

---

## 技術細節

### MCPWM Timer Config0 Register 結構

```
bits [31:0]: MCPWM_TIMERn_CFG0_REG

bits [7:0]:   timer_prescale (實際分頻 = value + 1)
bits [23:8]:  timer_period (實際週期 = value + 1)  ← 16-bit 完整 period 值
bits [25:24]: timer_period_upmethod
              00 = 立即更新
              01 = TEZ 同步 (Timer Equals Zero)
              10 = 同步至 in/out event
bits [31:26]: 保留位元
```

### 正確的 Mask 計算

目標：保留 bits[31:24] 和 bits[7:0]，清除 bits[23:8]

```
0xFFFF00FF = 1111 1111 1111 1111 0000 0000 1111 1111
             [31:24]  [23:16]  [15:8]   [7:0]
             保留     清除     清除     保留
```

### Shadow Register 更新流程

```cpp
// 1. 準備新的 cfg0 值（設定 bit[24]=1 啟用 TEZ 同步）
uint32_t cfg0_new = (cfg0_before & 0xFFFF00FF)  // 保留控制位元
                  | (((period - 1) & 0xFFFF) << 8);  // 寫入新 period

// 2. 寫入 register
MCPWM1.timer[0].timer_cfg0.val = cfg0_new;

// 3. 硬體行為：
//    - 新 period 值寫入 shadow register
//    - 等待下一個 TEZ 事件（timer 歸零時）
//    - TEZ 發生時，shadow register → 實際 register
//    - 結果：頻率變更完全無 glitch
```

---

## 測試計劃

### 測試環境
- **硬體**: ESP32-S3-DevKitC-1 N16R8
- **測試點**: GPIO17 (TX1 / UART1_TX)
- **測量工具**: 示波器或頻率計

### 測試項目

#### 1. 基本頻率設定測試
```
命令: UART1 PWM 1000 50
預期: 1000 Hz, 50% duty cycle
測量: 頻率 = ______ Hz (應為 1000 Hz ±1%)

命令: UART1 PWM 10000 50
預期: 10000 Hz, 50% duty cycle
測量: 頻率 = ______ Hz (應為 10000 Hz ±1%)

命令: UART1 PWM 100000 50
預期: 100000 Hz, 50% duty cycle
測量: 頻率 = ______ Hz (應為 100000 Hz ±1%)
```

#### 2. 頻率切換測試（Glitch 檢測）
```
序列:
1. UART1 PWM 20000 50
2. UART1 PWM 10000 50
3. UART1 PWM 20000 50

觀察:
- 使用示波器捕捉切換瞬間波形
- 檢查是否有 glitch、毛刺或異常脈衝
- 預期：smooth transition，無 glitch
```

#### 3. 邊界值測試
```
最低頻率: UART1 PWM 10 50
預期: 10 Hz (測量 = ______ Hz)

最高頻率: UART1 PWM 500000 50
預期: 500 kHz (測量 = ______ Hz)
```

#### 4. Duty Cycle 準確度測試
```
命令: UART1 PWM 10000 25
預期: 10000 Hz, 25% duty
測量: Duty = ______%

命令: UART1 PWM 10000 75
預期: 10000 Hz, 75% duty
測量: Duty = ______%
```

### 預期結果
- ✅ 所有頻率測量值在 ±1% 誤差範圍內
- ✅ Duty cycle 準確度在 ±2% 範圍內
- ✅ 頻率切換無 glitch 或異常波形
- ✅ Debug 輸出顯示 register 寫入成功

---

## 相關檔案

### 修改的檔案
- `src/UART1Mux.cpp` - 修改 `setPWMFrequencyAndDuty()` 函數

### 相關函數
- `updatePWMRegistersDirectly()` - 統一的 register 更新函數
- `calculatePrescalerAndPeriod()` - 計算最佳 prescaler 和 period
- `handleUART1Commands()` - 命令解析器

### 參考文件
- ESP32-S3 Technical Reference Manual - MCPWM Chapter
- `CLAUDE.md` - "MCPWM Glitch-Free PWM Updates" section

---

## 版本歷史

### v2.x.x (2025-01-11) - 待發布
**修復**: PWM 頻率輸出錯誤
- 修正 `setPWMFrequencyAndDuty()` 的 register mask 錯誤
- 改用 `updatePWMRegistersDirectly()` 確保 critical section 保護
- 確保 16-bit period 值完整寫入
- 保持 shadow register mode 的無 glitch 更新特性

### 相關 Commits
- 待測試通過後建立 commit

---

## 後續工作

### 優先級 1: 測試驗證
- [ ] 執行完整測試計劃
- [ ] 記錄測量結果
- [ ] 驗證 glitch-free 特性

### 優先級 2: 文件更新
- [ ] 更新 `CLAUDE.md` 的 MCPWM 章節
- [ ] 記錄此次 bug 修復作為案例研究
- [ ] 更新測試腳本

### 優先級 3: 程式碼品質
- [ ] 移除 `setPWMFrequencyAndDuty()` 中的冗餘 debug 程式碼
- [ ] 考慮重構：是否所有頻率更新都應該走統一路徑
- [ ] 檢查其他直接寫 register 的地方是否有相同問題

---

## 附錄 A: Debug 輸出範例

### 修復前（錯誤）
```
[DEBUG] handleUART1PWM: Calling setPWMFrequencyAndDuty(1000 Hz, 50.0%)
[UART1] 📖 cfg0_before=0x001F3F09
[UART1] 🔧 Writing cfg0=0x001F3F09
[UART1] 📖 cfg0_after=0x001F3F09  ← 值沒改變！
UART1 PWM: 1000 Hz, 50.0% duty, enabled
```

### 修復後（預期）
```
[DEBUG] handleUART1PWM: Calling setPWMFrequencyAndDuty(1000 Hz, 50.0%)
[UART1] 📖 BEFORE: cfg0=0x001F3F09
[UART1]    period (bits[23:8])=8000
[UART1] 📖 AFTER:  cfg0=0x001F3F09
[UART1]    period (bits[23:8])=8000  ← 值正確
[UART1] 🧮 Expected frequency: 80MHz / (10 × 8000) = 1000 Hz ✓
UART1 PWM: 1000 Hz, 50.0% duty, enabled
```

---

## 附錄 B: 快速參考

### 測試命令
```bash
# 編譯
pio run

# 上傳（需先關閉 serial monitor）
pio run -t upload

# 監看輸出
pio device monitor
```

### 測試序列
```
INFO
UART1 PWM 1000 50
UART1 PWM 10000 50
UART1 PWM 100000 50
UART1 STATUS
```

### 常用 Git 命令
```bash
# 查看修改
git diff src/UART1Mux.cpp

# 建立 commit（測試通過後）
git add src/UART1Mux.cpp
git commit -m "fix: 修正 UART1 PWM 頻率輸出錯誤

- 修正 setPWMFrequencyAndDuty() 的 register mask (0xFF0000FF → 0xFFFF00FF)
- 改用 updatePWMRegistersDirectly() 確保 critical section 保護
- 確保 16-bit period 值完整寫入 bits[23:8]
- 保持 shadow register mode 無 glitch 更新特性"
```

---

**備註**: 此備忘錄會持續更新測試結果和發現的問題。

**最後更新**: 2025-01-11
**負責人**: Bill Wang
**狀態**: 🟡 待測試驗證
