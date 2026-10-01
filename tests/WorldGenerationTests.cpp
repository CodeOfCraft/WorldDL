#include "world/WorldGeneration.h"

#include <iostream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace {

void require(bool value, char const* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

} // namespace

int main() {
    try {
        auto const options = nlohmann::json::parse(worlddl::VoidFlatWorldLayers);
        require(options.at("encoding_version") == 4, "Flat options must use Bedrock's named-block encoding");
        require(options.at("biome_id").is_number_integer(), "A valid biome must be specified");
        require(options.at("structure_options").is_null(), "Void exports must not generate structures");
        auto const& layers = options.at("block_layers");
        require(layers.is_array() && !layers.empty(), "Empty layer lists may fall back to default flat terrain");
        for (auto const& layer : layers) {
            require(layer.at("block_name") == "minecraft:air", "Undownloaded terrain must contain only air");
            require(layer.at("block_data") == 0, "Air must have the default block data");
            require(layer.at("count").is_number_integer() && layer.at("count").get<int>() > 0,
                    "The air layer must have a valid positive thickness");
        }
        std::cout << "WorldDL void generation settings tests passed\n";
        return 0;
    } catch (std::exception const& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
