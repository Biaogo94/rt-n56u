# Padavan 固件项目

本仓库基于 `rt-n56u/Padavan` 固件树维护，面向 MediaTek/Ralink 老平台路由器，重点围绕以下目标持续整理：

- 以 `MT7621` 为主线平台做长期维护和功能收敛
- 保持当前 `Linux 3.4` 内核与私有无线驱动栈，不做高风险平台重写
- 改善构建体验、固件产物可追溯性、运行时控制面的可维护性
- 在保证稳定性的前提下，为部分机型补齐更现代的无线能力与配置入口

当前默认分支为 `master`，手动和自动构建入口统一使用 GitHub Actions：

- Build Firmware: <https://github.com/Biaogo94/rt-n56u/actions/workflows/build.yml>

## 项目现状

### 维护重点

- `MT7621` 为优先维护平台
- `MT7615/MT7915` 无线组合为高级 Wi-Fi/KVR 能力的优先支持对象
- `MT7612` 等旧组合保留兼容性支持，但不作为主要优化方向
- 构建链已经收敛到统一目标目录与生成式 workflow，避免在 workflow 中手写型号列表

### 当前固件特点

- 使用 `gorden5566/padavan` 汉化字典
- Aria2 Web 前端替换为 `AriaNg`
- 固件产物统一命名为 `Padavan-型号-内核版本.trx`
- Actions 产物默认包含：
  - `Padavan-型号-内核版本.trx`
  - `md5sum.txt`
  - `manifest.json`
- `manifest.json` 会记录目标型号、板级映射、SoC、无线组合、内核配置、发布层级和 git revision
- 手动构建支持：
  - 单型号构建
  - `ALL` 全型号构建
  - 通过插件别名字符串选择可选插件

### 无线能力说明

- 当前项目仍然基于 `Linux 3.4.x + 私有无线驱动`
- 部分 `MT7615/MT7915` 机型已接入 `11k/v/r` 相关配置入口
- 这类能力是否可用，取决于板级配置、驱动能力和具体机型，不等于所有机型统一支持

## 支持机型

除原始 Padavan 支持机型外，本仓库继续维护以下机型：

- `PSG1208`
- `PSG1218`
- `5K-W20` (USB)
- `OYE-001` (USB)
- `NEWIFI-MINI` (USB)
- `MI-MINI` (USB)
- `MI-3` (USB)
- `MI-3C`
- `MI-4`
- `MI-R3G` (USB)
- `MI-R4A`
- `MI-R3P` (USB)
- `HC5661A`
- `HC5761A` (USB)
- `HC5861B`
- `360P2` (USB)
- `MI-NANO`
- `MZ-R13`
- `MZ-R13P`
- `RT-AC1200GU` (USB)
- `XY-C1` (USB)
- `WR1200JS` (USB)
- `NEWIFI3` (USB)
- `B70` (USB)
- `A3004NS` (USB)
- `K2P`
- `K2P-USB` (USB)
- `JCG-836PRO` (USB)
- `JCG-AC860M` (USB)
- `DIR-882` (USB)
- `DIR-878`
- `MR2600` (USB)
- `WDR7300`
- `RM2100`
- `CR660x` (`CR6606` / `CR6608` / `CR6609`)
- `R2100`
- `JCG-Y2` (USB)
- `E8820V2` (USB)
- `ZTE_E8820S` (USB)
- `MSG1500` (USB)
- `R6220` (USB)
- `NETGEAR-CHJ` (`R6260` / `R6350` / `R6850` / `WAC124`)
- `NETGEAR-BZV` (`R6800` / `R6700-v2` / `R7200` / `Nighthawk AC2400`)

实际可选型号来源于：

- `trunk/configs/templates/`
- `.github/firmware-targets.json`

## 可选插件

README 中列出的“可选插件”现在已经接入 GitHub Actions 手动构建。  
当你在 Actions 里填写 `plugins` 时，系统会：

1. 先将这组“可选插件”全部重置为关闭
2. 再按你输入的插件别名逐个开启
3. 模板里的其他核心配置保持不变

也就是说，`plugins` 只覆盖下面这组可选项，不会把整份机型模板全部推倒重来。

### 插件别名与配置项

| 插件别名 | 配置项 |
| --- | --- |
| `curl` | `CONFIG_FIRMWARE_INCLUDE_CURL` |
| `scutclient` | `CONFIG_FIRMWARE_INCLUDE_SCUTCLIENT` |
| `gdut-drcom` | `CONFIG_FIRMWARE_INCLUDE_GDUT_DRCOM` |
| `dogcom` | `CONFIG_FIRMWARE_INCLUDE_DOGCOM` |
| `minieap` | `CONFIG_FIRMWARE_INCLUDE_MINIEAP` |
| `njit-client` | `CONFIG_FIRMWARE_INCLUDE_NJIT_CLIENT` |
| `napt66` | `CONFIG_FIRMWARE_INCLUDE_NAPT66` |
| `softether-vpnserver` | `CONFIG_FIRMWARE_INCLUDE_SOFTETHERVPN_SERVER` |
| `softether-vpnclient` | `CONFIG_FIRMWARE_INCLUDE_SOFTETHERVPN_CLIENT` |
| `softether-vpncmd` | `CONFIG_FIRMWARE_INCLUDE_SOFTETHERVPN_CMD` |
| `vlmcsd` | `CONFIG_FIRMWARE_INCLUDE_VLMCSD` |
| `ttyd` | `CONFIG_FIRMWARE_INCLUDE_TTYD` |
| `lrzsz` | `CONFIG_FIRMWARE_INCLUDE_LRZSZ` |
| `htop` | `CONFIG_FIRMWARE_INCLUDE_HTOP` |
| `nano` | `CONFIG_FIRMWARE_INCLUDE_NANO` |
| `iperf3` | `CONFIG_FIRMWARE_INCLUDE_IPERF3` |
| `dump1090` | `CONFIG_FIRMWARE_INCLUDE_DUMP1090` |
| `rtl-sdr` | `CONFIG_FIRMWARE_INCLUDE_RTL_SDR` |
| `samba3.6` | `CONFIG_FIRMWARE_INCLUDE_SMBD36` |
| `mtr` | `CONFIG_FIRMWARE_INCLUDE_MTR` |
| `socat` | `CONFIG_FIRMWARE_INCLUDE_SOCAT` |
| `srelay` | `CONFIG_FIRMWARE_INCLUDE_SRELAY` |
| `3proxy` | `CONFIG_FIRMWARE_INCLUDE_3PROXY` |
| `mentohust` | `CONFIG_FIRMWARE_INCLUDE_MENTOHUST` |
| `frpc` | `CONFIG_FIRMWARE_INCLUDE_FRPC` |
| `frps` | `CONFIG_FIRMWARE_INCLUDE_FRPS` |
| `tunsafe` | `CONFIG_FIRMWARE_INCLUDE_TUNSAFE` |
| `wireguard-go` | `CONFIG_FIRMWARE_INCLUDE_WIREGUARD` |
| `smartdns` | `CONFIG_FIRMWARE_INCLUDE_SMARTDNS` |

补充说明：

- Actions 支持输入插件别名，也支持直接输入上表里的 `CONFIG_FIRMWARE_INCLUDE_*`
- 输入多个插件时，使用逗号、空格或分号分隔都可以
- 例如：
  - `smartdns,ttyd,wireguard-go`
  - `smartdns ttyd wireguard-go`
  - `CONFIG_FIRMWARE_INCLUDE_SMARTDNS,CONFIG_FIRMWARE_INCLUDE_TTYD`

## 本地编译

### 1. 安装依赖

```shell
# Debian/Ubuntu
sudo apt update
sudo apt install unzip libtool-bin curl cmake gperf gawk flex bison nano xxd \
	fakeroot kmod cpio git python3-docutils gettext automake autopoint \
	texinfo build-essential help2man pkg-config zlib1g-dev libgmp3-dev \
	libmpc-dev libmpfr-dev libncurses5-dev libltdl-dev wget libc-dev-bin

# Archlinux/Manjaro
sudo pacman -Syu --needed git base-devel cmake gperf ncurses libmpc \
        gmp python-docutils vim rpcsvc-proto fakeroot cpio help2man

# Alpine
sudo apk add make gcc g++ cpio curl wget nano xxd kmod \
	pkgconfig rpcgen fakeroot ncurses bash patch \
	bsd-compat-headers python2 python3 zlib-dev \
	automake gettext gettext-dev autoconf bison \
	flex coreutils cmake git libtool gawk sudo

# CentOS 7
sudo yum update
sudo yum groupinstall "Development Tools"
sudo yum install ncurses-* flex byacc bison zlib-* texinfo gmp-* mpfr-* gettext \
	libtool* libmpc-* gettext-* python-docutils nano help2man fakeroot

# CentOS 8
sudo yum update
sudo yum groupinstall "Development Tools"
sudo yum install ncurses-* flex byacc bison zlib-* gmp-* mpfr-* gettext \
	libtool* libmpc-* gettext-* nano fakeroot
```

### 2. 克隆源码

```shell
git clone --depth=1 https://github.com/Biaogo94/rt-n56u.git /opt/rt-n56u
```

### 3. 准备工具链

```shell
cd /opt/rt-n56u/toolchain-mipsel

# 推荐：下载预编译工具链
sh dl_toolchain.sh

# 如需从源码重建工具链：
./clean_toolchain
./build_toolchain
```

### 4. 可选修改机型模板

```shell
nano /opt/rt-n56u/trunk/configs/templates/PSG1218.config
```

机型模板位于：

- `trunk/configs/templates/`

### 5. 开始编译

```shell
cd /opt/rt-n56u/trunk
fakeroot ./build_firmware_modify PSG1218
```

说明：

- `build_firmware_modify` 第一个参数是机型名，名称来自 `trunk/configs/templates/`
- 编译产物默认输出到 `trunk/images/`
- 首次编译完成后，如需切换机型，建议执行清理脚本

```shell
./clear_tree
```

### 6. 本地手工覆盖插件

如果你不想改模板文件，也可以在本地通过环境变量覆盖可选插件：

```shell
cd /opt/rt-n56u/trunk
export FIRMWARE_PLUGIN_RESET_LIST="CONFIG_FIRMWARE_INCLUDE_SMARTDNS CONFIG_FIRMWARE_INCLUDE_TTYD CONFIG_FIRMWARE_INCLUDE_WIREGUARD"
export FIRMWARE_PLUGIN_ENABLE_LIST="CONFIG_FIRMWARE_INCLUDE_SMARTDNS CONFIG_FIRMWARE_INCLUDE_TTYD"
fakeroot ./build_firmware_modify PSG1218
```

更推荐的方式仍然是：

- 长期配置改模板
- 临时实验用 GitHub Actions 的 `plugins` 输入

## GitHub Actions 手动构建

### 入口

```text
Actions -> Build Firmware -> Run workflow
```

### 可选输入

- `target`
  - 选择单个型号直接构建
  - 选择 `ALL` 按顺序构建全部型号
- `plugins`
  - 留空：保持该型号模板默认插件
  - 填写插件字符串：覆盖“可选插件”集合

### 输入示例

```text
target  = RM2100
plugins = smartdns,ttyd
```

### 错误处理

- 如果 `plugins` 中存在未知插件名，workflow 会在 `Resolve optional plugins` 步骤直接失败
- 不会进入依赖安装、工具链准备和真正编译阶段
- GitHub 日志里会显示无效插件名
- Job Summary 会列出所有支持的插件别名

### 产物说明

- 单型号构建：
  - `Padavan-型号-内核版本.trx`
  - `md5sum.txt`
  - `manifest.json`
- `ALL` 构建：
  - 一个 artifact 中包含全部 `.trx`
  - `md5sum.txt`
  - `manifest.json`

## 目录说明

- `toolchain-mipsel/`
  - 交叉工具链下载脚本和工具链工程
- `trunk/configs/templates/`
  - 机型模板配置，控制固件功能开关
- `trunk/configs/boards/`
  - 板级配置、内核配置、板卡头文件
- `trunk/build_firmware_modify`
  - 本地固件构建入口
- `trunk/build_firmware_ci`
  - CI 环境下的构建包装脚本
- `.github/firmware-targets.json`
  - 固件目标目录
- `.github/scripts/firmware_targets.py`
  - 目标目录与 `Build Firmware` workflow 的生成脚本

## 维护策略

- `master` 自动构建默认只做一个 smoke target，用于尽快发现编译回归
- 手动构建用于：
  - 指定型号
  - 指定插件组合
  - 构建全部型号
- Wi-Fi、KVR、运行时控制面优化优先落在 `MT7621` 及其主流无线组合上
- 老平台和高风险旧驱动组合继续保留，但按兼容性优先级维护

## 致谢

- 上游项目：<https://github.com/hanwckf/rt-n56u>
- 汉化字典：<https://github.com/gorden5566/padavan>
- 历史日志参考：<https://www.jianshu.com/p/d76a63a12eae>

## 参考链接

- <https://www.jianshu.com/p/cb51fb0fb2ac>
- <https://www.jianshu.com/p/6b8403cdea46>
