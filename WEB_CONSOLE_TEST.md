# Web Console 測試指南

## 快速測試 (5 分鐘)

### 第 1 步：上傳固件

```bash
cd D:/github/ESP32/fw/Motor_Control_V1
pio run -t upload
```

設備重啟後，LED 應顯示紫色（AP Mode 已啟用）。

### 第 2 步：打開 Web Console

在瀏覽器中訪問：
```
http://192.168.4.1/console.html
```

您應該看到：
- ✅ 頁面加載
- ✅ 頂部顯示 🟢 **已連接**（綠色）
- ✅ 歡迎消息
- ✅ 輸入框可用

### 第 3 步：發送測試命令

在命令輸入框中輸入以下命令，逐個按 Enter：

#### 命令 1: `INFO`
```
> INFO
```

**預期響應**:
```
裝置資訊:
晶片型號: ESP32-S3
自由記憶體: XXXXX bytes
Flash 大小: 16777216 bytes (16 MB)
...
```

#### 命令 2: `STATUS`
```
> STATUS
```

**預期響應**:
```
系統狀態:
系統正常運行中
已運行時間: X 秒
自由記憶體: XXXXX bytes
...
```

#### 命令 3: `HELP`
```
> HELP
```

**預期響應**:
```
可用命令列表:
基本命令:
  HELP                - 顯示此幫助信息
  INFO                - 顯示設備信息
  STATUS              - 顯示系統狀態
...
```

### 第 4 步：驗證響應

如果 Web Console 顯示了完整的響應，✅ **測試成功！**

如果沒有顯示響應，請：

1. **檢查 CDC 日誌** (打開 USB 連接的 CDC 監視器)
   - 應該看到: `[WS] 文本命令: INFO`
   - 應該看到: `[WS] 命令已處理: 是`
   - 應該看到: `[WS] 發送響應到客戶端 1: XXXX 字節`

2. **檢查瀏覽器控制台** (按 F12)
   - Network 標籤中，WebSocket 是否已連接？
   - Console 標籤中，是否有錯誤？

3. **查看完整的調試指南**: [WEB_CONSOLE_DEBUG.md](WEB_CONSOLE_DEBUG.md)

---

## 完整測試場景

### 場景 1: 基本命令測試

依次執行以下命令，驗證每個都有正確的響應：

```
HELP          # 應顯示完整的命令列表
INFO          # 應顯示設備信息
STATUS        # 應顯示系統狀態
*IDN?         # 應顯示: HID_ESP32_S3
```

### 場景 2: 馬達控制測試

```
SET PWM 10000 25       # 設定 PWM (頻率 10kHz, 佔空比 25%)
DELAY 2000             # 延遲 2 秒
RPM                    # 讀取 RPM
MOTOR STATUS           # 查看馬達狀態
```

### 場景 3: WiFi 命令測試

```
IP                     # 查看 IP 地址和網絡信息
AP STATUS              # 查看 AP Mode 狀態
WIFI CONNECT           # 使用保存的 WiFi 連接
```

### 場景 4: 設定和保存

```
SAVE                   # 保存所有設定
LOAD                   # 加載設定
RESET                  # 重置為出廠設定
```

### 場景 5: 命令歷史測試

1. 發送: `INFO`
2. 發送: `STATUS`
3. 發送: `HELP`
4. 按 **↑** 箭頭 3 次，應返回到 `INFO`
5. 按 **↓** 箭頭，應向下移動到 `STATUS`

---

## 性能測試

### 響應時間測試

測試不同命令的響應時間：

```
命令             | 預期延遲 | 允許偏差
-----------------|---------|--------
HELP            | < 100ms  | ±50ms
INFO            | < 100ms  | ±50ms
SET PWM         | < 50ms   | ±20ms
RPM             | < 50ms   | ±20ms
SAVE            | < 500ms  | ±200ms
LOAD            | < 500ms  | ±200ms
```

### 連接穩定性測試

1. 連接 Web Console
2. 連續發送 100 個簡單命令 (如 `STATUS`)
3. 驗證：
   - ✅ 所有命令都有響應
   - ✅ 沒有丟失的命令
   - ✅ 沒有亂碼
   - ✅ 沒有斷開連接

### 多用戶測試

1. 在兩個不同的瀏覽器標籤或設備上打開 Web Console
2. 在第一個標籤中發送: `SET PWM 20000 50`
3. 在第二個標籤中發送: `RPM`
4. 在第一個標籤中發送: `STATUS`
5. 驗證：
   - ✅ 兩個標籤都收到各自的響應
   - ✅ 命令不會相互干擾

---

## 故障排查測試

如果命令無響應，按以下順序測試：

### 測試 1: CDC 功能測試

連接 USB，在 CDC 監視器中執行相同的命令：

```
> INFO
```

**預期**: CDC 應顯示完整的響應

**如果 CDC 也無響應**:
→ 問題在後端 CommandParser，不在 Web Console
→ 檢查 src/CommandParser.cpp

**如果 CDC 有響應但 Web Console 沒有**:
→ 問題在 Web Console 或 WebSocket 集成
→ 查看 WEB_CONSOLE_DEBUG.md

### 測試 2: 連接測試

檢查 Web Console 是否真的已連接：

1. 打開 F12 (瀏覽器開發者工具)
2. 點擊 **Network** 標籤
3. 過濾 WebSocket (搜索 `ws:`)
4. 應該看到一個已連接的 WebSocket 連接
5. 發送命令並檢查 **Messages** 標籤

### 測試 3: 消息過濾測試

Web Console 應過濾掉自動廣播的狀態消息。

驗證步驟：
1. 打開 F12 → Console
2. 在控制台中執行：
   ```javascript
   ws.addEventListener('message', (e) => console.log(e.data))
   ```
3. 等待幾秒鐘觀察消息
4. 應該看到 JSON 狀態消息，但它們不會出現在 Web Console 中

### 測試 4: 命令格式測試

測試不同的命令格式：

```
HELP        ✅ 應工作
HELP        ✅ 應工作 (重複)
help        ❌ 不應工作 (小寫)
HELP ?      ❌ 不應工作 (額外字符)
 HELP       ✅ 應工作 (前導空格被自動去除)
HELP        ✅ 應工作 (尾部空格被自動去除)
```

---

## 驗證清單

在聲稱 Web Console 工作正常之前，驗證以下各項：

- [ ] Web Console 頁面加載無誤
- [ ] 顯示 🟢 **已連接** 狀態
- [ ] 輸入框可接收焦點
- [ ] 至少 3 個命令 (HELP, INFO, STATUS) 有完整響應
- [ ] 命令歷史正常工作 (↑↓ 箭頁)
- [ ] 沒有 JavaScript 錯誤 (F12 → Console)
- [ ] 瀏覽器可以連接到 WebSocket (F12 → Network)
- [ ] CDC 日誌顯示 [WS] 前綴的日誌
- [ ] 響應時間在預期範圍內 (<100ms)
- [ ] 設備不會因為 Web Console 而重啟

---

## 已知限制

在測試時，請注意以下已知限制：

1. **消息大小限制**: 超長的響應 (>1000 字符) 可能被截斷
2. **並發連接限制**: 最多 8 個同時 WebSocket 連接
3. **自動廣播**: 狀態消息每 200ms 廣播一次 (已在前端過濾)
4. **超時**: 沒有命令執行超時，長時間運行的命令可能導致超時

---

## 提交測試報告

如果您完成了所有測試，請提交報告：

### 成功報告
```
✅ 所有測試通過
- 設備: ESP32-S3-DevKitC-1 N16R8
- 固件版本: [your version]
- 瀏覽器: [browser name and version]
- 操作系統: [OS name and version]
- 連接方式: AP Mode / Station Mode
```

### 失敗報告
```
❌ 以下測試失敗:
- 測試項: [test name]
- 症狀: [what happened]
- 預期: [what should happen]
- 錯誤消息: [any error messages]
- CDC 日誌: [relevant CDC output]
- 瀏覽器控制台錯誤: [any JS errors]
```

---

## 快速參考

| 任務 | 命令 |
|------|------|
| 查看幫助 | `HELP` |
| 設備信息 | `INFO` |
| 系統狀態 | `STATUS` |
| 設定 PWM | `SET PWM 20000 50` |
| 讀取 RPM | `RPM` |
| 查看 IP | `IP` |
| 連接 WiFi | `WIFI SSID Password` |
| 保存設定 | `SAVE` |
| 加載設定 | `LOAD` |
| 重置設備 | `RESET` |

---

**祝你測試愉快！** 🎉

如有問題，參考 [WEB_CONSOLE_DEBUG.md](WEB_CONSOLE_DEBUG.md) 進行調試。
