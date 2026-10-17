<p align="center"><img src="logo.png" alt="WorldDL" width="160"></p>

# WorldDL

English | [简体中文](README.zh-CN.md)

Author: [CodeOfCraft](https://github.com/CodeOfCraft) · Discord: [join](https://discord.gg/NyjNnuG2Rc)

A Windows x64 Minecraft Bedrock client mod that saves received chunks as a local world. Built with [LeviLamina](https://github.com/LiteLDev/LeviLamina) on its [official mod template](https://github.com/LiteLDev/levilamina-mod-template).

## Compatibility

| Component | Supported version |
| --- | --- |
| Platform | Windows x64, Bedrock client |
| Minecraft Bedrock | 1.26.51 |
| LeviLamina | 26.51.6, client |
| Engine data | `bedrockdata 26.51.1-client.7` |
| WorldDL | 0.1.0 |

Java Edition, mobile clients, and Bedrock Dedicated Server (BDS) are unsupported. A Minecraft update needs matching LeviLamina dependencies and a review of the engine interfaces; changing the manifest alone is not enough.

## Install and use

1. Install the matching LeviLamina version with the [client installation guide](https://lamina.levimc.org/user_guides/install_on_client/).
2. Build the mod, or download a package from [Releases](https://github.com/CodeOfCraft/WorldDL/releases) and extract it. Copy the whole `WorldDL` directory into the game's `mods` directory, so that `mods/WorldDL/manifest.json` and `mods/WorldDL/WorldDL.dll` exist.
3. Join a world or server, run `/wdl start "World name"` in chat, and explore the areas to save.
4. Run `/wdl stop` and wait for the success message before copying or packaging the world.

Commands run locally on the client. The server does not need the mod, and server operator permissions are not required.

| Command | Action |
| --- | --- |
| `/wdl` or `/wdl status` | Show session state, written chunk count, queue, and output path |
| `/wdl start` | Start a new session with the default world name |
| `/wdl start "Name"` | Start a new session with a custom display name |
| `/wdl save` | Recapture loaded chunks and wait for writes, keeping the session open |
| `/wdl stop` | Perform the final capture, drain the queue, and close the database |

Each session creates its own directory at `mods/WorldDL/data/worlds/wdl_<timestamp>/`, containing `level.dat`, `level.dat_old`, `levelname.txt`, and `db/`.

After stopping, copy the whole world directory into the game's `minecraftWorlds` directory, or export a `.mcworld` archive with PowerShell 7:

```powershell
pwsh -File scripts/Export-World.ps1 -WorldPath "C:\path\to\wdl_<timestamp>"
# Optional destination; an existing file is never overwritten.
pwsh -File scripts/Export-World.ps1 -WorldPath "C:\path\to\wdl_<timestamp>" -OutputPath "C:\exports\world.mcworld"
```

The script checks the database lock and puts the world files at the archive root. Open the `.mcworld` file with Minecraft to import it. Stop downloading before exporting or opening the saved world.

## Saved data and limits

### Saved data

- Received subchunk blocks, extra block layers, biomes, heightmaps, and block entity NBT.
- Separate keys for the Overworld, Nether, and End, including negative coordinates and negative subchunk heights.
- The game's `SubChunk::serialize(..., false)` disk format instead of connection-specific block runtime IDs.
- Missing subchunks are skipped; a partial update keeps the subchunks saved before it.

### Capture and storage

- Subchunk loading notifications are coalesced and captured on client ticks, with a final capture before chunk destruction and a periodic scan every 20 ticks. Loaded notifications take priority.
- Normal capture handles up to 4 chunks per tick and checks a 2 ms budget between chunks; a single chunk can exceed that budget. The writer queue holds up to 256 pending snapshots.
- Owned byte buffers go to a background writer that uses uncompressed LevelDB and synchronous atomic batches. A 2 ms coalescing window combines up to 32 snapshots per commit, with a roughly 4 MiB batch target.
- Stopping, leaving the world, or disabling the mod closes the session.

### Limits

Only data received by the client can be saved; unexplored areas are not downloaded. Exported Overworld generation is configured as air-only, so missing Overworld terrain stays empty while saved chunks are preserved. Nether and End generation is independent of that setting. Exports use Creative mode with commands enabled.

Entities, player inventories, scoreboards, map data, scheduled ticks, server plugin data, and resource/behavior packs are not exported. Container contents may not be sent to the client and cannot be guaranteed. Custom blocks need their corresponding packs.

`/wdl save`, `/wdl stop`, and leaving the world wait for disk writes and may briefly pause the game. They drain pending writes when the queue fills and after capture completes; normal tick capture pauses while the writer queue is full. A queue rejection means the queue was full, and loaded chunks are retried during later scans. Before a chunk is destroyed, a rejected capture waits for pending writes and retries once. `/wdl status` reports snapshot write and batch counts.

## Build

Requirements: Windows x64, Visual Studio C++ build tools and Windows SDK, LLVM/clang-cl 22, xmake 3.0+, Git, and PowerShell 7 for the scripts. Dependencies are fetched from the official LeviMC xmake repository.

```powershell
pwsh -File scripts/Build.ps1
# Debug build
pwsh -File scripts/Build.ps1 -Mode debug
```

The helper uses xmake and LLVM from `PATH`, with optional portable tools under `.tools/`. It enables Git long paths only for the build process. A normal toolchain installation is enough; `.tools/` is not part of the repository.

Alternatively:

```powershell
xmake f -y -p windows -a x64 -m release --target_type=client
xmake -y
```

Output: `bin/WorldDL/`, a complete mod directory ready to copy or archive.

## Verification

Storage, format, and export tests do not need Minecraft. Install CMake 3.24+ and Visual Studio C++ build tools; the first configuration fetches LevelDB and nlohmann/json.

```powershell
cmake -S tests -B build/tests -A x64
cmake --build build/tests --config Release --parallel
ctest --test-dir build/tests -C Release --output-on-failure
pwsh -File tests/ExportWorldTests.ps1
```

Tests cover coordinate and dimension keys, negative heights, `level.dat` headers, void generation settings, database reopening, partial updates, concurrent batch updates, queue pressure, draining on stop, capture notification coalescing and chunk lifetimes, refusing existing world directories, and archive packaging. GitHub Actions builds the client and runs these checks.

In-game validation is still required: connect to a server, start a capture, explore, change blocks, switch dimensions, stop or disconnect, import the world, and inspect blocks and biomes. Automated tests do not establish runtime hook behavior or world-loading compatibility.

## Project layout

| Path | Responsibility |
| --- | --- |
| `src/mod/` | Mod lifecycle, event wiring, and session state |
| `src/hooks/` | Chunk load, chunk destruction, and world-leave hooks |
| `src/command/` | `/wdl` command overloads |
| `src/world/ChunkCapture.*` | Engine serialization and metadata capture |
| `src/world/WorldGeneration.*` | Air-only Overworld configuration |
| `src/world/WorldWriter.*` | Background LevelDB writes |
| `src/world/BedrockFormat.h` | World keys and file headers |
| `scripts/` | Build helper and offline export |
| `tests/` | Standalone storage, format, and export checks |

The build target and its packaging list live in [xmake.lua](xmake.lua); package metadata is in [tooth.json](tooth.json); `manifest.json` is the template that the packer expands into the built mod.

See [CONTRIBUTING.md](CONTRIBUTING.md) for development and release instructions, and [CHANGELOG.md](CHANGELOG.md) for changes.

## License

Project code retains the template's [CC0-1.0 license](LICENSE). Dependencies retain their own licenses; see [NOTICE.md](NOTICE.md) and [LICENSES/](LICENSES/).
