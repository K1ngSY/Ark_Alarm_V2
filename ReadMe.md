# Ark Alarm Bot V2

Ark Alarm Bot V2 是一个面向 Windows 的自动监控与报警工具，用于《方舟：生存飞升》。项目以 C++17 和 Qt 重新实现，通过 OCR 与图像处理实现无人值守的游戏守护功能。

## 功能概览

- **自动监控**：`Scanner` 模块定时截图游戏窗口，通过 Tesseract OCR 识别副栉龙提示及部落日志，并根据用户配置决定是否发送文本、图片或拨打电话。
- **崩溃处理**：`CrashHandler` 会检测崩溃弹窗，自动关闭并重新启动游戏。
- **自动重连**：`Rejoiner` 在崩溃后自动进入服务器，支持识别服务器是否包含 Mod 并在下载完成后继续进入。
- **消息发送**：`Sender` 负责与第三方通讯平台的窗口交互，模拟输入文字或图片并发送报警信息。
- **辅助工具**：
  - `motion.*` 提供鼠标键盘模拟。
  - `visual.*` 负责截图及图像处理。
  - `utility.*` 提供窗口查找与崩溃窗口检测等功能。
- **多语言**：项目预留了翻译文件 `Ark_Alarm_V2_zh_CN.ts`，程序启动时会加载对应的翻译。

## 目录结构

- `dashboard.*` - 主界面及 UI 逻辑。
- `scanner.*` - 监控与报警的核心实现。
- `crashhandler.*` - 游戏崩溃监测与重启。
- `rejoiner.*` - 自动重连服务器流程。
- `sender.*` - 向通讯平台窗口发送消息。
- `motion.*` - 模拟输入操作。
- `visual.*` - 截图、OCR 及图像识别。
- `utility.*` - 窗口相关的辅助函数。
- `util/` - 存放报警音效 (`Log_Alarm.wav`, `P_Alarm.wav`)。

## 依赖

- Qt（项目文件 `Ark_Alarm_V2.pro` 指定了 `core`、`gui`、`network`、`widgets`、`multimedia` 模块）
- OpenCV
- Tesseract OCR 与 Leptonica
- Windows SDK（使用 Win32 API）

将对应库的头文件与链接路径配置好后，可使用 `qmake` 或 Qt Creator 进行编译。

## 使用说明

1. 确保 `tessdata` 目录位于可执行文件同级目录下，内含 `chi_sim` 与 `eng` 训练数据。
2. 在界面中设置游戏窗口标题、通讯平台窗口标题及坐标等参数。
3. 启动监控后，程序会周期性截图并根据日志触发条件发送提醒。

## 许可

仓库未包含开源许可文件，默认保留所有权利。

