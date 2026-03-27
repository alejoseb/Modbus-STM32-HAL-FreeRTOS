# 贊助

如果您覺得這個函式庫有用，或已將其用於商業產品，請考慮支持我的工作或請我喝杯咖啡：

[:heart: 贊助](https://github.com/sponsors/alejoseb)

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/alejoseb)

[!["Paypal"](https://github.com/alejoseb/Modbus-STM32-HAL-FreeRTOS/blob/master/.github/paypal.png)](https://www.paypal.com/donate/?business=ADTEDK5JHVZ22&no_recurring=0&item_name=I+have+contributed+libraries+for+embedded.+If+you+find+any+of+my+open-source+projects+useful+please+consider+supporting+me.%0A&currency_code=USD)[Paypal](https://www.paypal.com/donate/?business=ADTEDK5JHVZ22&no_recurring=0&item_name=I+have+contributed+libraries+for+embedded.+If+you+find+any+of+my+open-source+projects+useful+please+consider+supporting+me.%0A&currency_code=USD)

我也在不同的贊助層級提供諮詢服務，感謝您的支持！


# STM32微控制器的Modbus函式庫
基於Cube HAL與FreeRTOS的STM32微控制器TCP、USART及USB-CDC Modbus RTU主站與從站函式庫。

包含多款熱門開發板的範例，包括BluePill、NUCLEO-64、NUCLEO-144及Discovery板（Cortex-M3/M4/M7）。

本專案移植自Arduino的Modbus函式庫：https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino

STM32F4-discovery板搭配TouchGFX的示範影片：https://youtu.be/XDCQvu0LirY

`新` 各資料型別擁有獨立記憶體區域及可配置的起始位址，並與舊版共享記憶體模式完全向下相容

`新` 基於Pymodbus的腳本測試範例

`新` TCP從站（伺服器）多客戶端，支援可配置的自動老化演算法管理TCP連線


## 社群翻譯版本：
繁體中文：[繁體中文](TraditionalChineseREADME.md)

## 特點：
- 可移植到ST Cube HAL支援的任何STM32 MCU。
- 可移植到其他微控制器，例如[Raspberry PI Pico](https://github.com/alejoseb/Modbus-PI-Pico-FreeRTOS)，只需少量的工程工作。
- 基於FreeRTOS的多執行緒安全實作。
- 多個Modbus實例（主站和/或從站）可在同一MCU中同時運行，僅受MCU可用UART/USART數量限制。
- 靈活的記憶體模型：共享記憶體空間（舊版）或各資料型別獨立記憶體區域，並支援可配置的Modbus起始位址。
- 支援RS232與RS485。
- 支援USART DMA以實現高波特率與空閒線路偵測。
- 支援F103 Bluepill板的USB-CDC RTU主站與從站。
- 支援TCP主站與從站，提供F429及H743 MCU的範例。


## 檔案結構
```
├── LICENSE
├── README.md
├── Examples
    ├── ModbusBluepill --> STM32F103C8 USART 從站
    ├── ModbusBluepillUSB --> STM32F103C8 USART + USB-CDC 主站與從站
    ├── ModbusF103 --> NUCLEO-F103RB Modbus 主站與從站
    ├── ModbusF429 --> NUCLEO-F429ZI Modbus 從站
    ├── ModbusF429TCP --> NUCLEO-F429ZI Modbus TCP
    ├── ModbusF429DMA --> NUCLEO-F429ZI Modbus RTU DMA 主站與從站
    ├── ModbusL152DMA --> NUCLEO-L152RE Modbus RTU DMA 從站
    ├── ModbusH743 --> NUCLEO-H743ZI Modbus 從站
    ├── ModbusH743TCP --> NUCLEO-H743ZI Modbus TCP
    ├── ModbusF303 --> NUCLEO-F303RE Modbus 從站
    ├── ModbusSTM32F4-discovery --> STM32F4-discovery TouchGFX + Modbus 主站
    ├── ModbusWB55DMA --> P-NUCLEO-WB55 Modbus RTU DMA 從站（RS485）
    ├── ModbusG070 --> NUCLEO-G070RB Modbus 從站
    ├── ModbusF030 --> STM32F030RCT6 USART 從站
    ├── ModbusH503 --> STM32H503RBTx USART 從站
    ├── ModbusG431 --> NUCLEO-G431KB USART 從站
├── Script
    ├── *.ipynb --> 各種用於測試的Jupyter筆記本（主站與從站）
├── MODBUS-LIB --> 函式庫資料夾
    ├── Inc
    │   └── Modbus.h
    ├── Config
    │   └── ModbusConfigTemplate.h --> 配置模板
    └── Src
        ├── Modbus.c
        └── UARTCallback.c

```

## 如何使用範例
範例適用於STM32CubeIDE版本：1.8.0 https://www.st.com/en/development-tools/stm32cubeide.html

- 從系統資料夾將範例匯入STM32Cube IDE
- 連接您的NUCLEO開發板
- 編譯並開始偵錯！
- 若需要調整鮑率或其他參數，建議使用Cube-MX助手。如果更換USART埠，需要為所選USART啟用中斷。詳情請查閱UARTCallback.c。

### 注意事項與已知問題：
- Modbus RTU USART標準中斷模式適用於115200 bps或更低的鮑率。對於更高的鮑率——已測試至2 Mbps——建議使用DMA模式。請查閱對應範例，Cube HAL的DMA通道需要額外配置。

- USB-CDC範例僅支援Bluepill開發板，尚未在其他開發板上驗證。使用此範例需要在ModbusConfig.h中啟用USB-CDC。

- TCP範例已在NUCLEO F429ZI與H743ZI上驗證。使用這些範例需要在ModbusConfig.h中啟用TCP。

- CubeMX產生的LWIP TCP HAL實作，若在連線前MCU已啟動，可能無法正常運作。這是已知問題，可依以下連結手動修改產生的程式碼解決：https://community.st.com/s/question/0D50X0000CDolzDSQR/ethernet-does-not-work-if-uc-starts-with-the-cable-disconnected

請查閱NUCLEO F429的TCP範例，其中已包含手動修改。

## 如何移植到自己的MCU
- 在STM32Cube IDE中為您的MCU建立新專案
- 在Cube-MX的中介軟體區段啟用FreeRTOS CMSIS_V2
- 配置USART並啟用全域中斷
- 若使用USART的DMA模式，請配置RX與TX的DMA請求
- 將USART中斷的`搶佔優先級`設定為低於FreeRTOS排程器的優先級（標準配置下為5或更大的數字），此參數在NVIC配置面板中修改
- 使用拖放方式將Modbus函式庫資料夾（MODBUS-LIB）從主機作業系統匯入STM32Cube IDE專案
- 出現提示時，選擇連結資料夾和檔案
- 更新專案屬性中的include路徑，加入MODBUS-LIB資料夾的`Inc`資料夾
- 使用ModbusConfigTemplate.h建立ModbusConfig.h，並將其加入專案的include路徑
- 宣告一個全域的modbusHandler_t並參考倉庫中提供的範例
- `注意：`若您的專案已將USART中斷服務用於其他用途，需相應修改UARTCallback.c


## 從站記憶體配置

函式庫支援兩種從站記憶體模型，均在`main.c`（或對應的應用程式檔案）中配置。

### 選項一 — 共享記憶體（舊版，向下相容）

所有功能碼（FC1/2/3/4/5/6/15/16）共用單一`uint16_t`陣列。線圈位址對應陣列的位元；暫存器位址對應陣列的字組。此為預設模式，現有程式碼無需任何修改。

```c
uint16_t ModbusDATA[8]; // 所有功能碼共用

ModbusH.uModbusType = MB_SLAVE;
ModbusH.port        = &huart2;
ModbusH.u8id        = 1;
ModbusH.u16timeOut  = 1000;
ModbusH.EN_Port     = NULL;
ModbusH.u16regs     = ModbusDATA;
ModbusH.u16regsize  = sizeof(ModbusDATA) / sizeof(ModbusDATA[0]);
ModbusH.xTypeHW     = USART_HW;
```

### 選項二 — 各資料型別獨立記憶體區域與可配置起始位址

每種資料型別擁有獨立的`uint16_t`陣列及專屬的Modbus起始位址。這可實現標準Modbus位址映射（例如：線圈從0開始、離散輸入從10001開始、保持暫存器從40001開始）或任意自訂分區。當任一區域指標被設定時，對應的功能碼將使用該專用區域，而非`u16regs`。

```c
// 各資料型別獨立陣列
uint16_t CoilsDATA[2];     // 32個線圈，Modbus位址   0 –  31
uint16_t DiDATA[2];        // 32個離散輸入，Modbus位址 100 – 131
uint16_t HoldingDATA[4];   // 4個保持暫存器，Modbus位址 200 – 203
uint16_t InputDATA[4];     // 4個輸入暫存器，Modbus位址 300 – 303

ModbusH.uModbusType = MB_SLAVE;
ModbusH.port        = &huart2;
ModbusH.u8id        = 1;
ModbusH.u16timeOut  = 1000;
ModbusH.EN_Port     = NULL;
ModbusH.u16regs     = NULL;  // 配置獨立區域時不使用此欄位
ModbusH.u16regsize  = 0;
ModbusH.xTypeHW     = USART_HW;

// 線圈 — FC1、FC5、FC15
ModbusH.u16coils            = CoilsDATA;
ModbusH.u16coilsStartAdd    = 0;
ModbusH.u16coilsNregs       = 2;

// 離散輸入 — FC2
ModbusH.u16discreteInputs          = DiDATA;
ModbusH.u16discreteInputsStartAdd  = 100;
ModbusH.u16discreteInputsNregs     = 2;

// 保持暫存器 — FC3、FC6、FC16
ModbusH.u16holdingRegs          = HoldingDATA;
ModbusH.u16holdingRegsStartAdd  = 200;
ModbusH.u16holdingRegsNregs     = 4;

// 輸入暫存器 — FC4
ModbusH.u16inputRegs          = InputDATA;
ModbusH.u16inputRegsStartAdd  = 300;
ModbusH.u16inputRegsNregs     = 4;
```

使用獨立區域時，每種資料型別僅回應其專屬的位址範圍，對範圍外的位址存取將回傳例外碼2（非法資料位址）。各區塊之間的記憶體完全隔離。

`Examples/ModbusG431`（NUCLEO-G431KB）提供使用獨立記憶體區域的完整可運行範例。

## 適用於Linux與Windows的推薦Modbus主從測試與開發工具

### 新！提供給AI代理與AI輔助開發的Modbus主站MCP伺服器
https://github.com/alejoseb/ModbusMCP


### 主站與從站Python函式庫

Linux/Windows：https://github.com/riptideio/pymodbus

### 主站客戶端 Qmodbus
Linux：    https://launchpad.net/~js-reynaud/+archive/ubuntu/qmodbus

Windows：  https://sourceforge.net/projects/qmodbus/

### 從站模擬器
Linux：https://sourceforge.net/projects/pymodslave/

Windows：https://sourceforge.net/projects/modrssim2/

## 待辦事項：
- ~~實作各資料型別的隔離記憶體空間~~  已實作各資料型別獨立記憶體區域及可配置起始位址，並與舊版共享記憶體模式完全向下相容。
- 為主站功能碼實作包裝函式。目前電報（telegram）需手動定義。
- 改善功能文件
- ~~改善資料接收佇列；目前方法過於繁重，應以簡單緩衝區、串流或其他FreeRTOS原語替代。~~ 已解決，佇列由環形緩衝區取代（03/19/2021）
- ~~使用RS485收發器進行測試（已實作但未測試）~~ 已通過MAX485收發器驗證（01/03/2021）
- ~~實作Modbus TCP~~ （28/04/2021）
- ~~改善TCP佇列以支援多客戶端及TCP工作階段管理~~ （10/24/2021）
