#include "hm/parser/parser.hpp"
#include "hm/process/process.hpp"
#include "hm/toolchain/compiler.hpp"

#include "hp/colors/color.hpp"
#include "hp/containers/containers.hpp"
#include "hp/other/other.hpp"
#include <filesystem>

static std::string envCompilerFor(const std::string &ext) {
    bool isCpp = ext == ".cpp" || ext == ".cxx" || ext == ".cc";
    bool isC = ext == ".c";

    if (!isCpp && !isC)
        return "";
    std::string cross;
    if (const char *cc = std::getenv("CROSS_COMPILE"); cc && *cc)
        cross = cc;
    if (isCpp) {
        if (const char *env = std::getenv("CXX"); env && *env)
            return env;
        if (!cross.empty())
            return cross + "g++";
    } else {
        if (const char *env = std::getenv("CC"); env && *env)
            return env;
        if (!cross.empty())
            return cross + "gcc";
    }
    return "";
}

int Parser::autoCompiler(Config &c, const std::string &tag) {
    if (!c.compiler.empty())
        return 0;

    struct Candidate {
        std::string base;
        std::string subcommand;
    };

    struct CompilerGroup {
        std::vector<std::string> extensions;
        std::vector<Candidate> candidates;
        std::string languageName;
    };

    static const std::vector<CompilerGroup> compilerGroups = {
        {{".c"}, {{"gcc", ""}, {"clang", ""}, {"zig", "cc"}}, "C"},
        {{".cpp", ".cxx", ".cc"}, {{"g++", ""}, {"clang++", ""}, {"cl", ""}, {"zig", "c++"}}, "C++"},
        {{".zig"}, {{"zig", ""}}, "Zig"},
        {{".go"}, {{"go", ""}}, "Go"},
        {{".rs"}, {{"rustc", ""}}, "Rust"},
        {{".py"}, {{"python3", ""}, {"python", ""}}, "Python"},
        {{".java"}, {{"javac", ""}}, "Java"},
        {{".cs"}, {{"csc", ""}, {"mcs", ""}}, "C#"},
        {{".ts"}, {{"tsc", ""}}, "TypeScript"},
        {{".js"}, {{"node", ""}}, "JavaScript"},
        {{".rb"}, {{"ruby", ""}}, "Ruby"},
        {{".php"}, {{"php", ""}}, "PHP"},
        {{".pl"}, {{"perl", ""}}, "Perl"},
        {{".swift"}, {{"swiftc", ""}}, "Swift"},
        {{".kt", ".kts"}, {{"kotlinc", ""}}, "Kotlin"},
        {{".hs"}, {{"ghc", ""}}, "Haskell"},
        {{".lua"}, {{"lua", ""}}, "Lua"},
        {{".r"}, {{"Rscript", ""}}, "R"},
        {{".jl"}, {{"julia", ""}}, "Julia"},
        {{".dart"}, {{"dart", ""}}, "Dart"},
        {{".scala"}, {{"scalac", ""}}, "Scala"},
        {{".clj"}, {{"clojure", ""}}, "Clojure"},
        {{".erl"}, {{"erlc", ""}}, "Erlang"},
        {{".ex", ".exs"}, {{"elixir", ""}}, "Elixir"},
        {{".fs", ".fsi", ".fsx"}, {{"fsharpc", ""}, {"dotnet", "fsi"}, {"dotnet", "fsc"}}, "F#"},
        {{".ml", ".mli"}, {{"ocamlc", ""}, {"ocamlopt", ""}}, "OCaml"},
        {{".vbs"}, {{"cscript", ""}}, "VBScript"},
        {{".bat", ".cmd"}, {{"cmd.exe", ""}}, "Batch Script"},
        {{".ps1"}, {{"powershell.exe", ""}, {"pwsh.exe", ""}}, "PowerShell"},
    };

    std::string firstExt;
    bool anyInput = false;

    if (c.v_inputFiles.empty()) {
        int rc = collectFiles(c, ".");
        if (rc != 0) {
            hp::printlnCl(std::format("{} Error: No input files found for auto-detecting compiler.", tag), hp::RED);
            return rc;
        }
    }

    for (const auto &file : c.v_inputFiles) {
        std::string ext = std::filesystem::path(file).extension().string();
        if (ext.empty())
            continue;

        if (!anyInput) {
            firstExt = ext;
            anyInput = true;
        }

        std::string envComp = envCompilerFor(ext);
        if (!envComp.empty()) {
            c.compiler = envComp;
            c.v_Info[std::format("{} Warning: Compiler not specified. Using env override: {}", tag, c.compiler)] = hp::YELLOW;
            return 0;
        }

        for (const auto &group : compilerGroups) {
            bool extMatches = false;
            for (const auto &e : group.extensions) {
                if (e == ext) {
                    extMatches = true;
                    break;
                }
            }
            if (!extMatches)
                continue;

            for (const auto &cand : group.candidates) {
                if (hm::Process::exists(cand.base)) {
                    c.compiler = cand.subcommand.empty() ? cand.base : (cand.base + " " + cand.subcommand);
                    c.v_Info[std::format("{} Warning: Compiler not specified. Auto-detected compiler: {}", tag, c.compiler)] = hp::YELLOW;
                    return 0;
                }
            }

            c.v_Errors.push_back(std::format("{} Error: No compiler found for '{}' files. Install one of: {}", tag, group.languageName,
                                             hm::Process::join([&] {
                                                 std::vector<std::string> names;
                                                 for (const auto &cand : group.candidates)
                                                     names.push_back(cand.subcommand.empty() ? cand.base : cand.base + " " + cand.subcommand);
                                                 return names;
                                             }())));
            return 1;
        }
    }

    if (!anyInput) {
        c.v_Info[std::format("{} Error: No input files to detect a compiler from.\n Set 'Compiler:' explicitly or add 'InputFiles {{ }}'.", tag)] = hp::RED;
        return 1;
    }

    c.v_Info[std::format("{} Error: No compiler known for extension '{}'. Set 'Compiler:' explicitly.", tag, firstExt)] = hp::RED;
    c.v_Errors.push_back(std::format("{} Error: No compiler known for extension '{}'. Set 'Compiler:' explicitly.", tag, firstExt));
    return 1;
}

int Parser::autoVersion(Config &c, const std::string &tag) {
    if (!c.version.empty())
        return 0;

    if (c.compiler == "cl" || c.compiler == "msvc") {
        c.version = "/std:c++latest";
        return 0;
    }

    bool isGcc = c.compiler == "gcc" || c.compiler == "g++";
    bool isClang = c.compiler == "clang" || c.compiler == "clang++";
    bool isZig = c.compiler == "zig cc" || c.compiler == "zig c++";

    if (!isGcc && !isClang && !isZig)
        return 0;

    std::string raw = trim(hp::command(std::format("{} -dumpversion", c.compiler)));
    if (raw.empty())
        return 0;

    int major = 0;
    std::sscanf(raw.c_str(), "%d", &major);
    if (major <= 0)
        return 0;

    bool isC = (c.compiler == "gcc" || c.compiler == "clang" || c.compiler == "zig cc");

    struct StdEntry {
        std::string flag;
        int gcc;
        int clang;
    };

    static constexpr std::array<StdEntry, 5> cxxMap = {{
        {"std=c++14", 5, 3},
        {"std=c++17", 8, 7},
        {"std=c++20", 11, 12},
        {"std=c++23", 14, 17},
        {"std=c++26", 16, 22},
    }};

    static constexpr std::array<StdEntry, 4> cMap = {{
        {"std=c99", 5, 3},
        {"std=c11", 8, 6},
        {"std=c17", 14, 18},
        {"std=c23", 14, 18},
    }};

    auto pick = [&](const auto &table, const std::string &fallback) -> std::string {
        std::string chosen = fallback;
        for (const auto &entry : table) {
            int required = isGcc ? entry.gcc : entry.clang;
            if (major >= required)
                chosen = entry.flag;
        }
        return chosen;
    };

    c.version = isC ? pick(cMap, "std=c99") : pick(cxxMap, "std=c++14");
    c.v_Info[std::format("{} Warning: Version not specified. Auto-detected version flag: {}", tag, c.version)] = hp::YELLOW;
    return 0;
}

int Parser::autoInputFiles(Config &c, const std::string &tag) {
    if (!c.v_inputFiles.empty())
        return 0;

    struct SourceEntry {
        std::vector<std::string> compilers;
        std::vector<std::string> extensions;
    };

    static const std::vector<SourceEntry> sourceExts = {
        {{"gcc", "clang", "zig cc"}, {".c"}},
        {{"g++", "clang++", "cl", "zig c++"}, {".cpp", ".cxx", ".cc"}},
        {{"zig"}, {".zig"}},
        {{"go"}, {".go"}},
        {{"rustc"}, {".rs"}},
        {{"python3", "python"}, {".py"}},
    };

    static constexpr std::array<std::string_view, 4> srcDirs{
        "src",
        "source",
        "code",
        "app",
    };

    static constexpr std::array<std::string_view, 32> excludedDirs = {
        ".git",
        ".hg",
        ".svn",
        ".vs",
        ".cache",
        ".idea",
        ".gitlab",
        ".github",
        "build",
        "bin",
        "obj",
        "out",
        "dist",
        "target",
        "node_modules",
        "vendor",
        "third_party",
        "external",
        "deps",
        "dep",
        "lib",
        "libs",
        "include",
        "includes",
        "test",
        "tests",
        "examples",
        "demos",
        "playground",
        "scratch",
        "tmp",
        "temp",
    };

    auto isSource = [&](const std::filesystem::path &p) {
        const auto ext = p.extension().string();
        return std::any_of(sourceExts.begin(), sourceExts.end(), [&](const SourceEntry &entry) {
            const bool compilerMatches = std::find(entry.compilers.begin(), entry.compilers.end(), c.compiler) != entry.compilers.end();
            const bool extensionMatches = std::find(entry.extensions.begin(), entry.extensions.end(), ext) != entry.extensions.end();
            return compilerMatches && extensionMatches;
        });
    };

    auto isExcluded = [&](const std::filesystem::path &p) {
        for (const auto &component : p)
            for (const auto &skip : excludedDirs)
                if (component.string() == skip)
                    return true;
        return false;
    };

    auto addFile = [&](const std::filesystem::path &p) {
        if (isSource(p) && !isExcluded(p))
            c.v_inputFiles.push_back(p.generic_string());
    };

    if (std::any_of(srcDirs.begin(), srcDirs.end(), [](std::string_view dir) -> bool {
            return !dir.empty() && std::filesystem::exists(dir) && std::filesystem::is_directory(dir);
        })) {

        std::error_code ec;
        for (const auto dirSrc : srcDirs) {
            if (dirSrc.empty())
                continue;
            for (auto it = std::filesystem::recursive_directory_iterator(dirSrc, std::filesystem::directory_options::skip_permission_denied, ec);
                 it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
                if (ec)
                    break;
                if (it->is_directory() && isExcluded(it->path())) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (it->is_regular_file())
                    addFile(it->path());
            }
        }

        for (const auto &entry : std::filesystem::directory_iterator(".", ec)) {
            if (ec)
                break;
            if (entry.is_regular_file())
                addFile(entry.path());
        }
    } else {
        std::error_code ec;
        for (auto it = std::filesystem::recursive_directory_iterator(".", std::filesystem::directory_options::skip_permission_denied, ec);
             it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
            if (ec)
                break;
            if (it->is_directory() && isExcluded(it->path())) {
                it.disable_recursion_pending();
                continue;
            }
            if (it->is_regular_file())
                addFile(it->path());
        }
    }

    if (!c.v_inputFiles.empty()) {
        std::vector<std::string> stems;
        for (const auto &f : c.v_inputFiles) {
            std::ifstream in(f);
            if (!in)
                continue;
            std::string line;
            while (std::getline(in, line)) {
                std::size_t pos = line.find("#include");
                if (pos == std::string::npos)
                    continue;

                std::size_t angleOpen = line.find('<', pos);
                std::size_t quoteOpen = line.find('"', pos);

                if (quoteOpen == std::string::npos)
                    continue;
                if (angleOpen != std::string::npos && angleOpen < quoteOpen)
                    continue;

                std::size_t q2 = line.find('"', quoteOpen + 1);
                if (q2 == std::string::npos)
                    continue;
                stems.push_back(std::filesystem::path(line.substr(quoteOpen + 1, q2 - quoteOpen - 1)).stem().string());
            }
        }
        hp::remove::duplicates(stems);

        if (!stems.empty()) {
            std::vector<std::string> original = c.v_inputFiles;
            std::error_code ec;
            for (auto it = std::filesystem::recursive_directory_iterator(".", std::filesystem::directory_options::skip_permission_denied, ec);
                 it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
                if (ec)
                    break;
                if (it->is_directory() && isExcluded(it->path())) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (!it->is_regular_file())
                    continue;
                if (!isSource(it->path()))
                    continue;
                if (std::ranges::find(stems, it->path().stem().string()) == stems.end())
                    continue;
                c.v_inputFiles.push_back(it->path().lexically_normal().generic_string());
            }
            hp::remove::duplicates(c.v_inputFiles);

            std::vector<std::string> added;
            for (const auto &f : c.v_inputFiles)
                if (std::ranges::find(original, f) == original.end())
                    added.push_back(f);

            if (!added.empty() && !c.quiet) {
                std::string msg = std::format("{} Warning: Auto-added companion sources:\n", tag);
                for (const auto &a : added)
                    msg += a;
                c.v_Info[msg] = hp::YELLOW;
            }
        }
    }

    if (c.v_inputFiles.empty()) {
        cfg.v_Info[std::format("{} Error: No input files found. Add an 'InputFiles {{ }}' block or place sources in 'src/'.", tag)] = hp::RED;
        c.v_Errors.push_back(std::format("{} Error: No input files found. Add an 'InputFiles {{ }}' block or place sources in 'src/'.", tag));
        return 1;
    }

    c.v_Info[std::format("{} Warning: Input Files not specified. Auto-detected Input Files.", tag)] = hp::YELLOW;

    return 0;
}

int Parser::collectFiles(Config &c, const std::string &folderPath) {
    if (!c.v_inputFiles.empty())
        return 0;

    std::filesystem::path root(folderPath);
    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root))
        return 1;

    static constexpr std::string_view sourceExts[] = {
        ".c", ".cpp", ".cxx", ".cc", ".zig", ".go", ".rs", ".py", ".java", ".cs", ".ts", ".js",
        ".rb", ".php", ".pl", ".swift", ".kt", ".kts", ".hs", ".lua", ".r", ".jl", ".dart", ".scala",
        ".clj", ".erl", ".ex", ".exs", ".fs", ".fsi", ".fsx", ".ml", ".mli", ".vbs", ".bat", ".cmd", ".ps1"};

    static constexpr std::array<std::string_view, 32> excludedDirs = {
        ".git",
        ".hg",
        ".svn",
        ".vs",
        ".cache",
        ".idea",
        ".gitlab",
        ".github",
        "build",
        "bin",
        "obj",
        "out",
        "dist",
        "target",
        "node_modules",
        "vendor",
        "third_party",
        "external",
        "deps",
        "dep",
        "lib",
        "libs",
        "include",
        "includes",
        "test",
        "tests",
        "examples",
        "demos",
        "playground",
        "scratch",
        "tmp",
        "temp",
    };

    auto isExcludedDir = [&](const std::filesystem::path &p) {
        for (const auto &component : p) {
            auto name = to_lower(component.string());
            for (const auto &skip : excludedDirs)
                if (name == skip)
                    return true;
        }
        return false;
    };

    std::map<std::string, std::vector<std::string>> extGroupedFiles;

    std::error_code ec;
    for (auto it = std::filesystem::recursive_directory_iterator(
             root, std::filesystem::directory_options::skip_permission_denied, ec);
         it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (ec)
            break;

        if (it->is_directory() && isExcludedDir(it->path())) {
            it.disable_recursion_pending();
            continue;
        }

        if (!it->is_regular_file())
            continue;

        std::string ext = to_lower(it->path().extension().string());
        if (std::ranges::find(sourceExts, ext) == std::end(sourceExts))
            continue;

        extGroupedFiles[ext].push_back(it->path().lexically_relative(root).generic_string());
    }

    if (extGroupedFiles.empty())
        return 1;

    auto maxExtIt = std::max_element(
        extGroupedFiles.begin(),
        extGroupedFiles.end(),
        [](const auto &a, const auto &b) {
            return a.second.size() < b.second.size();
        });

    size_t dominantCount = maxExtIt->second.size();
    const std::string dominantExt = maxExtIt->first;

    for (const auto &[ext, files] : extGroupedFiles) {
        if (ext == dominantExt || files.empty())
            continue;

        if ((dominantExt == ".c" || dominantExt == ".cpp" || dominantExt == ".cxx" || dominantExt == ".cc") &&
            (ext == ".c" || ext == ".cpp" || ext == ".cxx" || ext == ".cc"))
            continue;

        c.v_Info[std::format("[HelpMake] Warning: Found {} '{}' file{} alongside {} '{}' file{}. Compiling only '{}'. Use 'InputFiles {{ }}' to include specific files.",
                             files.size(), ext, files.size() == 1 ? "" : "s", dominantCount, dominantExt, dominantCount == 1 ? "" : "s",
                             dominantExt)] = hp::YELLOW;
    }

    c.v_inputFiles.insert(c.v_inputFiles.end(), maxExtIt->second.begin(), maxExtIt->second.end());
    return 0;
}

int Parser::autoIncludes(Config &c, const std::string &tag) {
    if (!c.includeFiles.empty())
        return 0;

    static constexpr std::array<std::string_view, 6> potentialIncludeDirs = {
        "include",
        "includes",
        "inc",
        "headers",
        "api",
        "public",
    };

    static constexpr std::array<std::string_view, 22> excludedDirs = {
        ".git",
        ".hg",
        ".svn",
        ".vs",
        ".cache",
        ".idea",
        ".gitlab",
        ".github",
        "build",
        "bin",
        "obj",
        "out",
        "dist",
        "target",
        "node_modules",
        "vendor",
        "third_party",
        "external",
        "deps",
        "dep",
        "test",
        "tests",

    };

    static constexpr std::array<std::string_view, 9> headerExts = {
        ".h",
        ".hpp",
        ".hxx",
        ".hh",
        ".inl",
        ".inc",
        ".ixx",
        ".cppm",
        ".tpp",
    };

    auto isExcludedDir = [&](const std::filesystem::path &p) {
        auto name = to_lower(p.filename().string());
        for (const auto &skip : excludedDirs)
            if (name == skip)
                return true;
        return false;
    };

    auto isNamedIncludeDir = [&](const std::filesystem::path &p) {
        auto name = to_lower(p.filename().string());
        for (const auto &inc : potentialIncludeDirs)
            if (name == inc)
                return true;
        return false;
    };

    auto hasHeaderExt = [&](const std::filesystem::path &p) {
        auto ext = to_lower(p.extension().string());
        for (const auto &e : headerExts)
            if (ext == e)
                return true;
        return false;
    };

    std::vector<std::string> headerDirs;

    std::error_code ec;
    for (auto it = std::filesystem::recursive_directory_iterator(".", std::filesystem::directory_options::skip_permission_denied, ec); it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (ec)
            break;

        const auto &path = it->path();

        if (it->is_directory() && isExcludedDir(path)) {
            it.disable_recursion_pending();
            continue;
        }

        if (it->is_directory() && isNamedIncludeDir(path)) {
            c.includeFiles.push_back(path.lexically_relative(".").generic_string());
            it.disable_recursion_pending();
            continue;
        }

        if (it->is_regular_file() && hasHeaderExt(path))
            headerDirs.push_back(path.parent_path().lexically_relative(".").generic_string());
    }

    if (c.includeFiles.empty())
        c.includeFiles = headerDirs;

    c.includeFiles = hp::remove::duplicates(c.includeFiles);

    if (c.includeFiles.empty())
        return 0;

    auto comp = hm::makeCompiler(c.compiler);
    for (const auto &dir : c.includeFiles) {
        if (!c.flags.empty())
            c.flags += ' ';
        c.flags += comp->includeFlag(dir);
    }

    std::string info = std::format("{} Warning: Include dirs not specified. Auto-detected include dirs:\n", tag);
    for (const auto &dir : c.includeFiles)
        info += dir;

    c.v_Info[info] = hp::YELLOW;

    return 0;
}

int Parser::autoLinkFlags(Config &c, const std::string &tag) {
    auto comp = hm::makeCompiler(c.compiler);
    if (!comp)
        return 0;

    bool useMsvc = comp->isMSVC();

    struct LinkEntry {
        std::string_view header;
        std::string_view gccFlag;
        std::string_view msvcFlag;
    };

    static constexpr std::array<LinkEntry, 23> linkTable = {{
        {"pthread.h", "-pthread", ""},
        {"thread", "-pthread", ""},
        {"mutex", "-pthread", ""},
        {"atomic", "-pthread", ""},
        {"condition_variable", "-pthread", ""},
        {"future", "-pthread", ""},
        {"cmath", "-lm", ""},
        {"math.h", "-lm", ""},
        {"dlfcn.h", "-ldl", ""},
        {"zlib.h", "-lz", "zlib.lib"},
        {"curl/curl.h", "-lcurl", "libcurl.lib"},
        {"openssl/ssl.h", "-lssl -lcrypto", "libssl.lib libcrypto.lib"},
        {"SDL2/SDL.h", "-lSDL2", "SDL2.lib"},
        {"GLFW/glfw3.h", "-lglfw", "glfw3.lib"},
        {"GL/gl.h", "-lGL", "opengl32.lib"},
        {"GL/glew.h", "-lGLEW", "glew32.lib"},
        {"gtest/gtest.h", "-lgtest -lgtest_main", "gtest.lib"},
        {"fmt/core.h", "-lfmt", "fmt.lib"},
        {"sqlite3.h", "-lsqlite3", "sqlite3.lib"},
        {"expat.h", "-lexpat", "expat.lib"},
        {"filesystem", "-lstdc++fs", ""},
        {"stacktrace", "-lstdc++_libbacktrace", ""},
        {"experimental/filesystem", "-lstdc++fs", ""},
    }};

    if (c.v_inputFiles.empty())
        return 0;

    std::vector<std::string> newFlags;
    for (const auto &f : expandInputs(c)) {
        std::ifstream in(f, std::ios::binary);
        if (!in)
            continue;
        std::string raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        std::string stripped = stripSource(raw);

        std::size_t pos = 0;
        while ((pos = stripped.find("#include", pos)) != std::string::npos) {
            std::size_t after = pos + 8;
            if (after < stripped.size() && stripped[after] != ' ' && stripped[after] != '\t' && stripped[after] != '<') {
                std::size_t nl = stripped.find('\n', pos);
                pos = (nl == std::string::npos) ? stripped.size() : nl + 1;
                continue;
            }

            while (after < stripped.size() && std::isspace(static_cast<unsigned char>(stripped[after])))
                ++after;

            if (after >= stripped.size() || stripped[after] != '<') {
                std::size_t nl = stripped.find('\n', pos);
                pos = (nl == std::string::npos) ? stripped.size() : nl + 1;
                continue;
            }

            std::size_t close = stripped.find('>', after + 1);
            if (close == std::string::npos)
                break;

            std::string value = stripped.substr(after + 1, close - after - 1);
            for (const auto &e : linkTable) {
                if (e.header != value)
                    continue;
                std::string_view flag = useMsvc ? e.msvcFlag : e.gccFlag;
                if (flag.empty())
                    break;
                newFlags.push_back(std::string(flag));
                break;
            }
            pos = close + 1;
        }
    }

    hp::remove::duplicates(newFlags);

    std::vector<std::string> actuallyAdded;
    for (const auto &f : newFlags) {
        if (c.flags.find(f) != std::string::npos)
            continue;
        if (!c.flags.empty())
            c.flags += ' ';
        c.flags += f;
        c.v_Flags.push_back(f);
        actuallyAdded.push_back(f);
    }

    if (!actuallyAdded.empty() && !c.quiet) {
        std::string msg = std::format("{} Warning: Auto-added link flags:\n", tag);
        for (const auto &f : actuallyAdded)
            msg += f;
        c.v_Info[msg] = hp::YELLOW;
    }
    return 0;
}

static bool hasEntryPoint(std::string_view src) {
    static constexpr std::array<std::string_view, 4> candidates = {
        "main",
        "WinMain",
        "wmain",
        "_tmain",
    };

    for (auto name : candidates) {
        std::size_t pos = 0;

        while ((pos = src.find(name, pos)) != std::string_view::npos) {
            bool startOk = pos == 0 || (!std::isalnum(static_cast<unsigned char>(src[pos - 1])) && src[pos - 1] != '_');
            std::size_t end = pos + name.size();
            bool endOk = end >= src.size() || (!std::isalnum(static_cast<unsigned char>(src[end])) && src[end] != '_');

            if (startOk && endOk) {
                std::size_t p = end;
                while (p < src.size() && std::isspace(static_cast<unsigned char>(src[p])))
                    ++p;
                if (p < src.size() && src[p] == '(')
                    return true;
            }
            pos = end;
        }
    }
    return false;
}

int Parser::autoKind(Config &c, const std::string &tag) {
    if (c.kind == "static")
        return 0;
    if (c.v_inputFiles.empty())
        collectFiles(c, tag);

    static constexpr std::array<std::string_view, 7> extEntry{
        ".c",
        ".cpp",
        ".cxx",
        ".cc",
        ".zig",
        ".go",
        ".rs",
    };

    bool hasCFamily = false;
    bool foundMain = false;

    for (const auto &file : expandInputs(c)) {
        std::string ext = to_lower(std::filesystem::path(file).extension().string());
        if (std::ranges::find(extEntry, ext) == extEntry.end())
            continue;
        hasCFamily = true;

        std::ifstream src(file, std::ios::binary);
        if (!src)
            continue;
        std::string raw((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
        std::string stripped = stripSource(raw);
        if (hasEntryPoint(stripped)) {
            foundMain = true;
            break;
        }
    }

    if (!hasCFamily) {
        c.kind = "executable";
        return 0;
    }

    c.kind = foundMain ? "executable" : "static";
    if (!foundMain)
        c.v_Info[std::format("{} Warning: No entry point found. Using Kind: static", tag)] = hp::YELLOW;
    return 0;
}

int Parser::autoOutput(Config &c, const std::string &tag) {
    if (!c.output.empty())
        return 0;

#ifdef _WIN32
    const std::string ext = ".exe";
#else
    const std::string ext = "";
#endif

    auto set = [&](const std::string &name) -> bool {
        if (name.empty() || name == "." || name == "/" || name == "\\" || name.front() == '.')
            return false;
        c.output = name + ext;
        return true;
    };

    if (std::filesystem::exists("CMakeLists.txt")) {
        std::ifstream in("CMakeLists.txt");
        std::string line;
        while (std::getline(in, line)) {
            std::size_t start = line.find("project(");
            if (start == std::string::npos)
                continue;
            start += 8;
            std::size_t end = line.find_first_of(" )", start);
            if (end == std::string::npos)
                continue;
            if (set(line.substr(start, end - start)))
                goto done;
        }
    }

    if (std::filesystem::exists("Cargo.toml")) {
        std::ifstream in("Cargo.toml");
        std::string line;
        bool inPackage = false;
        while (std::getline(in, line)) {
            std::string t = trim(line);
            if (t == "[package]") {
                inPackage = true;
                continue;
            }
            if (!t.empty() && t.front() == '[')
                inPackage = false;
            if (!inPackage)
                continue;
            if (!t.starts_with("name ") && !t.starts_with("name="))
                continue;
            std::size_t q1 = t.find('"');
            if (q1 == std::string::npos)
                continue;
            std::size_t q2 = t.find('"', q1 + 1);
            if (q2 == std::string::npos)
                continue;
            if (set(t.substr(q1 + 1, q2 - q1 - 1)))
                goto done;
        }
    }

    if (std::filesystem::exists("go.mod")) {
        std::ifstream in("go.mod");
        std::string line;
        while (std::getline(in, line)) {
            if (!line.starts_with("module "))
                continue;
            std::string mod = trim(line.substr(7));
            std::size_t slash = mod.find_last_of('/');
            if (set(slash == std::string::npos ? mod : mod.substr(slash + 1)))
                goto done;
        }
    }

    if (std::filesystem::exists("package.json")) {
        std::ifstream in("package.json");
        std::string line;
        while (std::getline(in, line)) {
            std::size_t pos = line.find("\"name\"");
            if (pos == std::string::npos)
                continue;
            std::size_t colon = line.find(':', pos);
            if (colon == std::string::npos)
                continue;
            std::size_t q1 = line.find('"', colon);
            if (q1 == std::string::npos)
                continue;
            std::size_t q2 = line.find('"', q1 + 1);
            if (q2 == std::string::npos)
                continue;
            if (set(line.substr(q1 + 1, q2 - q1 - 1)))
                goto done;
        }
    }

    if (std::filesystem::exists("pyproject.toml")) {
        std::ifstream in("pyproject.toml");
        std::string line;
        bool inProject = false;
        while (std::getline(in, line)) {
            std::string t = trim(line);
            if (t == "[project]" || t == "[tool.poetry]") {
                inProject = true;
                continue;
            }
            if (!t.empty() && t.front() == '[')
                inProject = false;
            if (!inProject)
                continue;
            if (!t.starts_with("name ") && !t.starts_with("name="))
                continue;
            std::size_t q1 = t.find_first_of("\"'");
            if (q1 == std::string::npos)
                continue;
            char quote = t[q1];
            std::size_t q2 = t.find(quote, q1 + 1);
            if (q2 == std::string::npos)
                continue;
            if (set(t.substr(q1 + 1, q2 - q1 - 1)))
                goto done;
        }
    }

    if (std::filesystem::exists("setup.py")) {
        std::ifstream in("setup.py");
        std::string line;
        while (std::getline(in, line)) {
            std::size_t pos = line.find("name=");
            if (pos == std::string::npos)
                continue;
            std::size_t q1 = line.find_first_of("\"'", pos);
            if (q1 == std::string::npos)
                continue;
            char quote = line[q1];
            std::size_t q2 = line.find(quote, q1 + 1);
            if (q2 == std::string::npos)
                continue;
            if (set(line.substr(q1 + 1, q2 - q1 - 1)))
                goto done;
        }
    }

    if (std::filesystem::exists("build.zig")) {
        std::ifstream in("build.zig");
        std::string line;
        while (std::getline(in, line)) {
            if (line.find("addExecutable") == std::string::npos)
                continue;
            std::size_t pos = line.find(".name");
            if (pos == std::string::npos)
                continue;
            std::size_t q1 = line.find('"', pos);
            if (q1 == std::string::npos)
                continue;
            std::size_t q2 = line.find('"', q1 + 1);
            if (q2 == std::string::npos)
                continue;
            if (set(line.substr(q1 + 1, q2 - q1 - 1)))
                goto done;
        }
    }

    if (std::filesystem::exists("meson.build")) {
        std::ifstream in("meson.build");
        std::string line;
        while (std::getline(in, line)) {
            std::size_t pos = line.find("project(");
            if (pos == std::string::npos)
                continue;
            std::size_t q1 = line.find('\'', pos);
            if (q1 == std::string::npos)
                continue;
            std::size_t q2 = line.find('\'', q1 + 1);
            if (q2 == std::string::npos)
                continue;
            if (set(line.substr(q1 + 1, q2 - q1 - 1)))
                goto done;
        }
    }

    if (std::filesystem::exists("pom.xml")) {
        std::ifstream in("pom.xml");
        std::string line;
        while (std::getline(in, line)) {
            std::size_t start = line.find("<artifactId>");
            if (start == std::string::npos)
                continue;
            start += 12;
            std::size_t end = line.find("</artifactId>", start);
            if (end == std::string::npos)
                continue;
            if (set(line.substr(start, end - start)))
                goto done;
        }
    }

    if (std::filesystem::exists("build.gradle") || std::filesystem::exists("build.gradle.kts")) {
        std::ifstream in(std::filesystem::exists("build.gradle") ? "build.gradle" : "build.gradle.kts");
        std::string line;
        while (std::getline(in, line)) {
            std::size_t pos = line.find("rootProject.name");
            if (pos == std::string::npos)
                continue;
            std::size_t q1 = line.find_first_of("\"'", pos);
            if (q1 == std::string::npos)
                continue;
            char quote = line[q1];
            std::size_t q2 = line.find(quote, q1 + 1);
            if (q2 == std::string::npos)
                continue;
            if (set(line.substr(q1 + 1, q2 - q1 - 1)))
                goto done;
        }
    }

    {
        std::error_code ec;
        for (const auto &e : std::filesystem::directory_iterator(".", ec)) {
            if (ec)
                break;
            if (!e.is_regular_file())
                continue;
            std::string ext2 = e.path().extension().string();
            if (ext2 != ".csproj" && ext2 != ".fsproj" && ext2 != ".vbproj")
                continue;
            if (set(e.path().stem().string()))
                goto done;
        }
    }

    for (const auto &f : expandInputs(c)) {
        if (std::filesystem::path(f).stem() != "main")
            continue;
        if (set("main"))
            goto done;
    }

    {
        std::string cwdName = std::filesystem::current_path().filename().string();
        std::string lower = to_lower(cwdName);
        static constexpr std::array<std::string_view, 7> generic = {
            "build",
            "out",
            "dist",
            "bin",
            "tmp",
            "temp",
            "src",
        };

        bool isGeneric = false;
        for (auto g : generic) {
            if (g == std::string_view(lower)) {
                isGeneric = true;
                break;
            }
        }
        if (!cwdName.empty() && !isGeneric && set(cwdName))
            goto done;
    }

#ifdef _WIN32
    c.output = "a.exe";
#else
    c.output = "a.out";
#endif
    if (!c.quiet)
        c.v_Info[std::format("{} Warning: Output not specified. Using default {}", tag, c.output)] = hp::YELLOW;
    return 0;

done:
    c.output = trim(c.output);
    if (!c.quiet)
        c.v_Info[std::format("{} Warning: Output not specified. Auto-detected {}", tag, c.output)] = hp::YELLOW;
    return 0;
}

void Parser::normalizeOutput(Config &c) {
    if (c.output.empty())
        return;

    std::filesystem::path p(c.output);

    if (p.has_extension())
        return;

    auto comp = hm::makeCompiler(c.compiler);
    bool isMsvc = comp && comp->isMSVC();

    std::string name = p.filename().string();
    std::string dir = p.parent_path().string();
    if (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
        dir += std::filesystem::path::preferred_separator;

    if (c.kind == "static") {
        if (isMsvc) {
            c.output = dir + name + ".lib";
        } else {
            if (!name.starts_with("lib"))
                name = "lib" + name;
            c.output = dir + name + ".a";
        }
    } else {
#ifdef _WIN32
        if (!isMsvc && !name.ends_with(".exe") && !name.ends_with(".scr") && !name.ends_with(".com"))
            c.output = dir + name + ".exe";
#else
        c.output = dir + name;
#endif
    }
}

int Parser::autoModules(Config &c, const std::string &tag) {
    if (!c.v_Modules.empty())
        return 0;

    std::vector<std::string> foundModules;
    std::regex importRegex(R"(^\s*(?:export\s+)?import\s+([a-zA-Z0-9_.:]+)\s*;)");

    for (const auto &file : expandInputs(c)) {
        std::ifstream in(file);
        if (!in)
            continue;
        std::string line;
        while (std::getline(in, line)) {
            std::smatch match;
            if (std::regex_search(line, match, importRegex)) {
                std::string modName = match[1].str();
                if (modName.starts_with("<") || modName == "std")
                    continue;

                std::string m_baseName = modName.substr(modName.find_last_of(".:") + 1);
                std::vector<std::string> exts = {".cppm", ".ixx", ".mpp", ".cxxm", ".c++m", ".ccm"};

                for (const auto &ext : exts) {
                    for (const auto &f : expandInputs(c)) {
                        std::filesystem::path p(f);
                        if (p.stem() == m_baseName && p.extension() == ext) {
                            foundModules.push_back(f);
                            break;
                        }
                    }
                }
            }
        }
    }

    c.v_Modules = hp::remove::duplicates(foundModules);

    if (!c.v_Modules.empty() && !c.quiet) {
        std::string msg = std::format("{} Warning: Modules not specified. Auto-detected Modules:\n", tag);
        for (const auto &m : c.v_Modules)
            msg += "  - " + m + "\n";
        c.v_Info[msg] = hp::YELLOW;
    }
    return 0;
}