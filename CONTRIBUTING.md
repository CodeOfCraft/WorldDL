# Contributing / 参与贡献

English and Chinese issues and pull requests are welcome. Read [README.md](README.md) or [README.zh-CN.md](README.zh-CN.md) for supported versions, build commands, and known limitations.

欢迎使用中文或英文提交 Issue 和 PR。支持版本、构建命令及已知限制见 [README.zh-CN.md](README.zh-CN.md) 或 [README.md](README.md)。

## Development / 开发

- Keep changes focused and follow the existing C++ style and `.clang-format`.
- Run the release build and the CMake/PowerShell checks listed in the README. For changes to hooks, capture, or world loading, also test in Minecraft and describe the results.
- Add focused regression coverage when changing storage or export behavior.
- Update both READMEs together when commands, requirements, or limitations change.
- Keep generated builds, downloaded tools, worlds, credentials, and personal paths out of commits. Code contributions use CC0-1.0; preserve notices for third-party code.

- 保持改动范围明确，遵循现有 C++ 风格与 `.clang-format`。
- 运行 README 中的 release 构建及 CMake/PowerShell 检查。修改 hook、捕获逻辑或存档加载行为时，还需在 Minecraft 中验证并说明结果。
- 修改存储或导出行为时，补充对应的回归测试。
- 命令、依赖或限制变化时同步更新两份 README。
- 不提交构建产物、下载工具、世界存档、凭据或个人路径。代码贡献采用 CC0-1.0；引入第三方代码时保留许可声明。

## Reporting Problems / 反馈问题

Include Minecraft, LeviLamina, and WorldDL versions, reproduction steps, expected and actual behavior, and relevant logs. State whether the issue occurs during capture, export, or import. Remove personal information and server details from logs before sharing.

请提供 Minecraft、LeviLamina 和 WorldDL 版本、复现步骤、预期与实际行为，以及相关日志，并说明问题发生在捕获、导出还是导入阶段。分享日志前移除个人信息及服务器信息。

## Releases / 发布

Follow the [official release guide](https://lamina.levimc.org/zh/developer_guides/tutorials/create_your_first_mod/#发布你的模组):

1. Set the same version in `tooth.json`, the `modVersion` rule in `xmake.lua`, and the startup log. `manifest.json` is a build template; its `${modVersion}` is expanded during packaging.
2. Update `CHANGELOG.md` and both READMEs. Verify the `tooth` repository identity and logo URL, and keep its asset URL aligned with the workflow's `WorldDL-client-windows-x64.zip` filename.
3. Build, run the automated checks, and complete the in-game validation described in the README.
4. Create a GitHub release from the intended commit using a tag such as `v0.1.0`. The leading `v` and semantic version are required for Bedrinth/LeviLauncher indexing. Actions builds, tests, packages, and uploads the archive.

Declare the LeviLamina client requirement in `tooth.json`. LeviLamina is a preload-native loader and is excluded from its ordinary mod dependency graph; adding it to `manifest.json` dependencies prevents WorldDL from loading.

参照[官方发布指南](https://lamina.levimc.org/zh/developer_guides/tutorials/create_your_first_mod/#发布你的模组)：

1. 同步 `tooth.json`、`xmake.lua` 的 `modVersion` 及启动日志中的版本。`manifest.json` 是构建模板，其中 `${modVersion}` 会在打包时替换。
2. 更新 `CHANGELOG.md` 与双语 README。核对 `tooth` 仓库标识及 logo 地址，下载 URL 的文件名应与工作流的 `WorldDL-client-windows-x64.zip` 一致。
3. 完成构建、自动化检查和 README 中的游戏内验证。
4. 从目标提交创建 GitHub Release，标签使用 `v0.1.0` 等格式。Bedrinth/LeviLauncher 收录要求以 `v` 开头且符合语义化版本。Actions 会构建、测试、打包并上传压缩包。

LeviLamina 客户端版本要求应写入 `tooth.json`。LeviLamina 是 preload-native 加载器，不参与普通模组依赖图；将它加入 `manifest.json` 的 dependencies 会阻止 WorldDL 加载。
