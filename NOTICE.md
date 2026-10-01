# Third-Party Notices / 第三方声明

WorldDL is based on the [LeviLamina mod template](https://github.com/LiteLDev/levilamina-mod-template) and retains its CC0-1.0 license in [LICENSE](LICENSE). The project's license does not replace the licenses of its dependencies.

WorldDL 基于 [LeviLamina 模组模板](https://github.com/LiteLDev/levilamina-mod-template)，沿用 [LICENSE](LICENSE) 中的 CC0-1.0 许可证。项目许可证不替代依赖各自的许可证。

| Dependency / 依赖 | Use / 用途 | License reference / 许可来源 |
| --- | --- | --- |
| [LeviLamina](https://github.com/LiteLDev/LeviLamina) 26.51.6 | Client mod loader and engine interfaces / 客户端加载器及引擎接口 | [Upstream license](https://github.com/LiteLDev/LeviLamina/blob/main/LICENSE) |
| [LevelDB](https://github.com/google/leveldb) 1.23 | Statically linked world database / 静态链接的存档数据库 | BSD-3-Clause, [full notice](LICENSES/LevelDB.txt) |
| [Snappy](https://github.com/google/snappy) 1.2.2 | LevelDB's linked compression dependency; world writes disable compression / LevelDB 链接的压缩依赖，存档写入禁用压缩 | BSD-3-Clause, [code license](LICENSES/Snappy.txt) |
| [fmt](https://github.com/fmtlib/fmt) 11.2.0 | Formatting through LeviLamina APIs / LeviLamina API 使用的格式化库 | MIT with optional exception, [full notice](LICENSES/fmt.txt) |
| [nlohmann/json](https://github.com/nlohmann/json) 3.12.0 | Test-only JSON parsing / 仅用于测试的 JSON 解析 | [MIT](https://github.com/nlohmann/json/blob/v3.12.0/LICENSE.MIT) |
| [levibuildscript](https://github.com/LiteLDev/xmake-repo) 0.6.1 | Build and packaging rules / 构建与打包规则 | See the package source and its upstream notices / 见包源码及上游声明 |

The toolchain fetches additional LeviLamina dependencies; the loader and runtime libraries are installed separately by the user. Keep their upstream license materials when redistributing them. Snappy benchmark datasets are not bundled. Minecraft is a separate proprietary dependency and is not included in this repository or release packages.

工具链会下载 LeviLamina 的其他依赖；加载器及运行时库由用户另行安装。重新分发时应保留上游许可材料。发布包不包含 Snappy 的基准测试数据集。Minecraft 为独立的专有软件依赖，不包含在本仓库或发布包中。
