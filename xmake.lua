add_rules("mode.debug", "mode.release")
set_xmakever("3.0.0")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

-- Keep these in sync with tooth.json; the CI workflows verify the pairing.
local MOD_VERSION    = "0.1.0"
local LOADER_VERSION = "26.51.6"

option("target_type")
    set_default("client")
    set_showmenu(true)
    set_values("client")
option_end()

add_requires("levilamina " .. LOADER_VERSION, {configs = {target_type = get_config("target_type")}})
add_requires("leveldb 1.23", {configs = {shared = false}})

add_requires("levibuildscript 0.6.1")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

-- Copied next to the built mod so that bin/<mod>/ is a complete, shippable directory.
local PACKAGE_FILES   = {
    "LICENSE",
    "NOTICE.md",
    "README.md",
    "README.zh-CN.md",
    "CONTRIBUTING.md",
    "CHANGELOG.md",
    "tooth.json",
    "logo.png",
}
local PACKAGE_DIRS    = {"LICENSES"}
local PACKAGE_SCRIPTS = {"Export-World.ps1"}

target("WorldDL")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker", {modVersion = MOD_VERSION})
    if is_plat("windows") then
        add_defines("NOMINMAX", "UNICODE")
        set_exceptions("none") -- To avoid conflicts with /EHa.
        add_cxflags( "/EHa", "/utf-8", "/W4", "/w44265", "/w44289", "/w44296", "/w45263", "/w44738", "/w45204")
        add_cxflags(
            "/EHs",
            "-Wno-microsoft-cast",
            "-Wno-invalid-offsetof",
            "-Wno-c++2b-extensions",
            "-Wno-microsoft-include",
            "-Wno-overloaded-virtual",
            "-Wno-ignored-qualifiers",
            "-Wno-missing-field-initializers",
            "-Wno-potentially-evaluated-expression",
            "-Wno-pragma-system-header-outside-header",
            {tools = {"clang_cl"}}
        )
        set_toolchains("clang-cl")
    end
    add_packages("levilamina", "leveldb")
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")
    add_headerfiles("src/**.h")
    add_files("src/**.cpp")
    add_includedirs("src")

    after_build(function(target)
        local root   = os.projectdir()
        local output = path.join(root, "bin", target:name())
        os.mkdir(output)
        for _, file in ipairs(PACKAGE_FILES) do
            os.cp(path.join(root, file), output)
        end
        for _, dir in ipairs(PACKAGE_DIRS) do
            os.cp(path.join(root, dir), output)
        end
        local scripts = path.join(output, "scripts")
        os.mkdir(scripts)
        for _, script in ipairs(PACKAGE_SCRIPTS) do
            os.cp(path.join(root, "scripts", script), scripts)
        end
    end)
