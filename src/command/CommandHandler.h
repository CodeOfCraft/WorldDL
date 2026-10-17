#pragma once

namespace worlddl {

class WorldDLMod;

/// Registers the `/wdl` command and its overloads with the client command registrar.
void registerCommands(WorldDLMod& mod);

} // namespace worlddl
