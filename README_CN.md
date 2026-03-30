# PeterSerialTerminal - Arduino 增强型串口终端库

基于 [ErriezSerialTerminal](https://github.com/Erriez/ErriezSerialTerminal) 的增强版 Arduino 串口终端库，新增命令历史记录、Tab 自动补全、更大缓冲区和现代化的交互式命令行体验。

![Serial Terminal](https://raw.githubusercontent.com/Peter/PeterSerialTerminal/master/extras/ScreenshotSerialTerminal.png)


## 新增特性

本增强版将你的 Arduino 串口终端提升为现代化、用户友好的命令行界面：

| 特性 | ErriezSerialTerminal | PeterSerialTerminal |
|---|---|---|
| 接收缓冲区大小 | 32 字节 | **256 字节** |
| 最大命令长度 | 8 字符 | **12 字符** |
| 命令历史记录 | 无 | **支持（最多 20 条）** |
| 方向键导航 | 无 | **支持（上/下键）** |
| Tab 自动补全 | 无 | **支持** |
| 历史管理 API | 无 | **支持** |
| 退格处理 | 基础 | **增强（BS + DEL）** |
| 行编辑与光标控制 | 无 | **支持** |
| 向后兼容 | - | **100%** |

### 命令历史记录

使用方向键浏览已输入过的命令：
- **上箭头（↑）**：回溯上一条命令
- **下箭头（↓）**：前进到下一条命令

最多可存储 20 条命令（每条最长 128 字节）。连续重复的命令会自动跳过，不会重复记录。

### Tab 自动补全

按下 **Tab** 键即可自动补全命令：
- **唯一匹配**：命令自动补全
- **多个匹配**：列出所有匹配的命令
- **无匹配**：向终端发送响铃字符

### 增强的终端编辑

- 改进的**退格**支持（同时处理 `^H` 和 `DEL` / ASCII 127）
- 自动**字符回显**，适配 PuTTY 等终端程序
- **行清除**和**光标控制**，编辑体验更流畅
- **命令后处理函数**，可自定义提示符（如 `> `）

### 内置历史管理

通过代码控制命令历史：

```c++
term.showHistory();   // 显示所有已存储的命令
term.clearHistory();  // 清空历史记录
term.addToHistory("command"); // 手动添加命令到历史记录
```


## 支持的硬件

**Arduino 系列：**
- UNO、Nano、Micro
- Pro / Pro Mini
- Mega / Mega2560
- Leonardo

**其他平台：**
- Arduino DUE
- ESP8266 / ESP32
- SAMD21
- STM32F1


## 快速入门

### 安装

1. 下载或克隆本仓库
2. 将文件夹复制到 Arduino 库目录
3. 重新启动 Arduino IDE

### 基础示例

```c++
#include <PeterSerialTerminal.h>

SerialTerminal term('\r', ' ');

void setup()
{
    Serial.begin(115200);

    term.setSerialEcho(true);
    term.setDefaultHandler(unknownCommand);
    term.setPostCommandHandler(printPrompt);

    term.addCommand("help", cmdHelp);
    term.addCommand("on", cmdLedOn);
    term.addCommand("off", cmdLedOff);

    pinMode(LED_BUILTIN, OUTPUT);
    Serial.println(F("Type 'help' for usage."));
    printPrompt();
}

void loop()
{
    term.readSerial();
}

void printPrompt()
{
    Serial.print(F("> "));
}

void unknownCommand(const char *command)
{
    Serial.print(F("Unknown command: "));
    Serial.println(command);
}

void cmdHelp()
{
    Serial.println(F("Commands: help, on, off"));
}

void cmdLedOn()
{
    Serial.println(F("LED on"));
    digitalWrite(LED_BUILTIN, HIGH);
}

void cmdLedOff()
{
    Serial.println(F("LED off"));
    digitalWrite(LED_BUILTIN, LOW);
}
```


## API 参考

### 构造函数

```c++
SerialTerminal term(newlineChar, delimiterChar);
```

| 参数 | 默认值 | 说明 |
|---|---|---|
| `newlineChar` | `'\n'` | 换行符（`'\r'` 或 `'\n'`） |
| `delimiterChar` | `' '` | 命令与参数之间的分隔符 |

### 命令注册

```c++
term.addCommand("cmd", callbackFunction);   // 注册命令
term.setDefaultHandler(unknownHandler);      // 未识别命令的处理函数
term.setPostCommandHandler(promptFunction);  // 每条命令执行后调用
```

### 串口读写

```c++
term.readSerial();         // 处理串口数据（在 loop() 中调用）
term.setSerialEcho(true);  // 启用字符回显
```

### 参数解析

```c++
char *arg = term.getNext();       // 获取下一个以空格分隔的参数
char *rest = term.getRemaining(); // 获取剩余的所有字符
```

### 缓冲区与历史记录

```c++
term.clearBuffer();           // 清空串口接收缓冲区
term.showHistory();           // 将命令历史打印到 Serial
term.clearHistory();          // 清空所有历史记录
term.addToHistory("cmd");     // 手动添加命令到历史记录
```


## 库配置

以下宏定义位于 `PeterSerialTerminal.h` 中，可根据需要调整：

| 宏定义 | 默认值 | 说明 |
|---|---|---|
| `ST_RX_BUFFER_SIZE` | 256 | 串口接收缓冲区大小（字节） |
| `ST_NUM_COMMAND_CHARS` | 12 | 命令名称最大字符数 |
| `ST_MAX_HISTORY_ENTRIES` | 20 | 最大历史记录条数 |
| `ST_HISTORY_ENTRY_SIZE` | 128 | 每条历史记录最大长度（字节） |


## 示例

| 示例 | 说明 |
|---|---|
| [ErriezSerialTerminal](examples/ErriezSerialTerminal/ErriezSerialTerminal.ino) | 基础用法：命令注册与参数解析 |
| [ErriezSerialTerminal_EchoAndCallback](examples/ErriezSerialTerminal_EchoAndCallback/ErriezSerialTerminal_EchoAndCallback.ino) | 字符回显、命令后处理函数与交互式提示符 |


## 依赖

无。


## 许可证

MIT 许可证。详见 [LICENSE](LICENSE)。


## 致谢

本项目基于 Erriez 团队开发的 [ErriezSerialTerminal](https://github.com/Erriez/ErriezSerialTerminal)。原始库为 Arduino 串口命令解析提供了一个优秀且轻量的基础框架。本增强版在该坚实基础上增加了命令历史记录、Tab 自动补全、扩展缓冲区和改进的终端编辑功能——同时保持与原有代码的完全向后兼容。在此衷心感谢 Erriez 团队在原始库上所做的出色工作。
