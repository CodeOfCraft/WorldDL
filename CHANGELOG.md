# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0]

### Added

- Add a LeviLamina 26.51.6 client mod for saving received Bedrock chunks.
- Add local start, save, stop and status commands.
- Write block palettes, biomes and block entities to an uncompressed LevelDB world.
- Add bounded background writes, separate dimension keys and session isolation.
- Add storage tests and an offline .mcworld packaging script.
- Configure air-only Overworld generation for undownloaded terrain.
- Retry the final capture when the write queue is full.
- Add English and Simplified Chinese documentation, contribution guidelines and dependency notices.
