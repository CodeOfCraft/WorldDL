<p align="center"><img src="logo.png" alt="WorldDL" width="160"></p>

# WorldDL

[English](README.md) | 简体中文

作者：[CodeOfCraft](https://github.com/CodeOfCraft)

将客户端收到的区块保存为本地世界的 Windows x64 Minecraft 基岩版模组。基于 [LeviLamina](https://github.com/LiteLDev/LeviLamina) 及其[官方模组模板](https://github.com/LiteLDev/levilamina-mod-template)。

## 兼容性

| 组件 | 支持版本 |
| --- | --- |
| 平台 | Windows x64，基岩版客户端 |
| Minecraft 基岩版 | 1.26.51 |
| LeviLamina | 26.51.6，client |
| 引擎数据 | `bedrockdata 26.51.1-client.7` |
| WorldDL | 0.1.0 |

不支持 Java 版、移动端或基岩版专用服务器（BDS）。升级 Minecraft 时，需要同步升级 LeviLamina 依赖并核对引擎接口，不能只修改 manifest 的版本号。

## 安装与使用

1. 按[客户端安装文档](https://lamina.levimc.org/user_guides/install_on_client/)安装对应版本的 LeviLamina。
2. 编译模组，或从 [Releases](https://github.com/CodeOfCraft/WorldDL/releases) 下载并解压可用的发布包。将整个 `WorldDL` 目录放入游戏的 `mods` 目录，确保存在 `mods/WorldDL/manifest.json` 和 `mods/WorldDL/WorldDL.dll`。
3. 进入世界或服务器，在聊天栏执行 `/wdl start "世界名称"`，然后探索需要保存的区域。
4. 执行 `/wdl stop`，等待成功消息后再复制或打包存档。

这些命令在客户端本地执行，无需服务器安装模组，也不需要服务器 OP 权限。

| 命令 | 作用 |
| --- | --- |
| `/wdl` 或 `/wdl status` | 显示会话状态、已写入区块数、队列与存档位置 |
| `/wdl start` | 开始新会话，使用默认世界名称 |
| `/wdl start "名称"` | 开始新会话，指定游戏中显示的名称 |
| `/wdl save` | 重新捕获已加载区块并等待写入，继续下载 |
| `/wdl stop` | 最后捕获、排空队列并关闭数据库 |

每次会话创建独立目录：`mods/WorldDL/data/worlds/wdl_<时间戳>/`，包含 `level.dat`、`level.dat_old`、`levelname.txt` 和 `db/`。

停止后，可以将整个存档目录复制到游戏的 `minecraftWorlds` 目录，也可以使用 PowerShell 7 导出 `.mcworld`：

```powershell
pwsh -File scripts/Export-World.ps1 -WorldPath "C:\path\to\wdl_<时间戳>"
# 可指定输出位置；不会覆盖已有文件。
pwsh -File scripts/Export-World.ps1 -WorldPath "C:\path\to\wdl_<时间戳>" -OutputPath "C:\exports\world.mcworld"
```

脚本检查数据库锁，并将存档文件直接放在压缩包根目录。使用 Minecraft 打开 `.mcworld` 即可导入。导出或打开存档前，请先停止下载。

## 保存范围与限制

- 保存已收到的子区块方块、附加方块层、生物群系、高度图和方块实体 NBT。
- 主世界、下界、末地使用独立坐标键，支持负坐标和负高度子区块。
- 在子区块加载、区块销毁前及周期扫描时捕获。默认每 20 tick 扫描一轮，每 tick 最多处理 4 个区块，队列最多容纳 256 个快照。
- 使用游戏自身 `SubChunk::serialize(..., false)` 的磁盘格式，避免保存仅在当前连接有效的方块 runtime ID。跳过未收到的子区块，部分更新保留此前已保存的其他子区块。
- 向后台写入线程传递独立内存的字节串，使用无压缩 LevelDB 和同步原子批量写入。停止、退出世界或禁用模组时关闭会话。

客户端只能保存实际收到的数据，未探索区域不会下载。导出世界的主世界生成器配置为纯空气，未下载区域保持空白，已保存区块保持原样。下界和末地的生成器独立，不受此设置控制。导出世界使用创造模式并开启命令。

实体、玩家背包、计分板、地图数据、计划刻、服务器插件数据和资源/行为包目前不导出。容器物品可能未由服务器发送给客户端，不能保证完整恢复。自定义方块需要另行安装对应资源/行为包。

`/wdl save`、`/wdl stop` 和退出世界会等待磁盘写入，可能造成短暂停顿。queue rejections 表示队列已满，仍加载的区块会在后续扫描中重试。区块销毁前若捕获被拒绝，会等待写入并重试一次。

## 编译

需要 Windows x64、Visual Studio C++ 构建工具及 Windows SDK、LLVM/clang-cl 22、xmake 3.0+、Git，以及运行脚本所需的 PowerShell 7。依赖从官方 LeviMC xmake 仓库下载。

```powershell
pwsh -File scripts/Build.ps1
# 调试构建
pwsh -File scripts/Build.ps1 -Mode debug
```

脚本优先使用 `PATH` 中的 xmake 和 LLVM，也支持 `.tools/` 下的便携工具，仅为构建进程启用 Git 长路径支持。常规安装的工具链即可构建，`.tools/` 不属于仓库内容。

也可以直接使用 xmake：

```powershell
xmake f -y -p windows -a x64 -m release --target_type=client
xmake -y
```

输出目录为 `bin/WorldDL/`。

## 验证

存储、格式及导出测试不需要 Minecraft。请安装 CMake 3.24+ 和 Visual Studio C++ 构建工具，首次配置时会下载 LevelDB 与 nlohmann/json。

```powershell
cmake -S tests -B build/tests -A x64
cmake --build build/tests --config Release --parallel
ctest --test-dir build/tests -C Release --output-on-failure
pwsh -File tests/ExportWorldTests.ps1
```

测试覆盖坐标与维度键、负高度、`level.dat` 文件头、虚空生成配置、数据库重开、部分更新、队列压力、停止时排空、拒绝覆盖已有存档，以及存档打包。GitHub Actions 会构建客户端并运行这些检查。

仍需游戏内验证：连接服务器、开始下载、探索区块、修改方块、切换维度、停止或断开连接、导入存档并检查方块与生物群系。自动化测试不能证明 hook 的运行时行为或客户端实际加载存档的兼容性。

## 源码结构

| 路径 | 职责 |
| --- | --- |
| `src/mod/` | 命令、事件、hook 和会话生命周期 |
| `src/world/ChunkCapture.*` | 引擎序列化与元数据捕获 |
| `src/world/WorldGeneration.*` | 纯空气主世界配置 |
| `src/world/WorldWriter.*` | 后台 LevelDB 写入 |
| `src/world/BedrockFormat.h` | 存档键和文件头 |
| `scripts/` | 构建脚本与离线导出 |
| `tests/` | 独立存储、格式及导出测试 |

开发及发布流程见 [CONTRIBUTING.md](CONTRIBUTING.md)，包元数据见 [tooth.json](tooth.json)，版本变化见 [CHANGELOG.md](CHANGELOG.md)。

## 许可证

项目代码沿用模板的 [CC0-1.0 许可证](LICENSE)。依赖保留各自许可证，见 [NOTICE.md](NOTICE.md) 和 [LICENSES/](LICENSES/)。
