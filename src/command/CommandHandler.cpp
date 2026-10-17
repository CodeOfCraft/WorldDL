#include "command/CommandHandler.h"

#include <string>

#include "ll/api/command/Command.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"

#include "mod/WorldDLMod.h"

namespace worlddl {

// Must have external linkage: the command overload reflects its fields through boost::pfr.
struct StartParams {
    std::string name;
};

void registerCommands(WorldDLMod& mod) {
    auto& command = ll::command::CommandRegistrar::getClientInstance()
                        .getOrCreateCommand("wdl", "WorldDL", CommandPermissionLevel::Any);
    command.overload().execute([&mod](CommandOrigin const&, CommandOutput& output) { mod.showStatus(output); });
    command.overload().text("status").execute([&mod](CommandOrigin const&, CommandOutput& output) {
        mod.showStatus(output);
    });
    command.overload().text("start").execute([&mod](CommandOrigin const&, CommandOutput& output) {
        mod.startSession({}, output);
    });
    command.overload<StartParams>().text("start").required("name").execute(
        [&mod](CommandOrigin const&, CommandOutput& output, StartParams const& params) {
            mod.startSession(params.name, output);
        }
    );
    command.overload().text("save").execute([&mod](CommandOrigin const&, CommandOutput& output) {
        mod.saveSession(output);
    });
    command.overload().text("stop").execute([&mod](CommandOrigin const&, CommandOutput& output) {
        mod.stopSession(output);
    });
}

} // namespace worlddl
