#pragma once

namespace worlddl::hooks {

/// Installs the chunk capture hooks. Calling it twice has no effect.
void install();

/// Removes the chunk capture hooks installed by install().
void uninstall();

} // namespace worlddl::hooks
