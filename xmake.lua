add_rules("mode.debug", "mode.release")
set_xmakever("3.0.0")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

option("target_type")
    set_default("client")
    set_showmenu(true)
    set_values("client")
option_end()

add_requires("levilamina 26.51.6", {configs = {target_type = "client"}})
add_requires("leveldb 1.23", {configs = {shared = false}})

add_requires("levibuildscript 0.6.1")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("WorldDL")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker", {modVersion = "0.1.0"})
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
        local root = os.projectdir()
        local output = path.join(root, "bin", target:name())
        os.mkdir(output)
        for _, file in ipairs({"LICENSE", "NOTICE.md", "README.md", "README.zh-CN.md", "CONTRIBUTING.md", "CHANGELOG.md", "tooth.json", "logo.png"}) do
            os.cp(path.join(root, file), output)
        end
        os.cp(path.join(root, "LICENSES"), output)
        local scripts = path.join(output, "scripts")
        os.mkdir(scripts)
        os.cp(path.join(root, "scripts", "Export-World.ps1"), scripts)
    end)
