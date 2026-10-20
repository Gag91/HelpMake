#include "hm/parser/parser.hpp"
#include "hm/util/strings.hpp"

#include <cstdlib>
#include <format>
#include <optional>
#include <string_view>
#include <vector>

std::optional<std::string> Parser::fieldValue(const std::string &line, std::string_view key) {
    auto pos = line.find(key);
    if (pos == std::string::npos)
        return std::nullopt;
    return cfg.var.parseVar(trim(line.substr(pos + key.size())), cfg.v_Errors);
}

void Parser::appendInputFiles(Config &c, const std::string &value) {
    for (const auto &tok : splitArgs(value)) {
        if (!c.inputFile.empty())
            c.inputFile += " ";
        c.inputFile += tok;
        c.v_inputFiles.push_back(tok);
    }
}

void Parser::appendIncludes(Config &c, const std::string &value) {
    for (const auto &tok : splitArgs(value)) {
        c.flags += " -I" + tok;
        c.includeFiles.push_back(tok);
    }
}

void Parser::appendFlags(Config &c, const std::string &value) {
    for (const auto &tok : splitArgs(value)) {
        if (!c.flags.empty())
            c.flags += " ";
        c.flags += tok;
        c.v_Flags.push_back(tok);
    }
}

void Parser::appendModules(Config &c, const std::string &value) {
    for (const auto &tok : splitArgs(value)) {
        if (!c.modules.empty())
            c.modules += " ";
        c.modules += tok;
        c.v_Modules.push_back(tok);
    }
}

void Parser::appendPreBuild(Config &c, const std::string &value) {
    if (c.Precmd.empty())
        c.Precmd = value;
    else
        c.Precmd += " && " + value;
    c.v_Precmd.push_back(value);
}

void Parser::appendPostBuild(Config &c, const std::string &value) {
    if (c.Postcmd.empty())
        c.Postcmd = value;
    else
        c.Postcmd += " && " + value;
    c.v_Postcmd.push_back(value);
}

void Parser::setKind(Config &c, const std::string &rawValue, std::size_t lineNo) {
    c.kind = to_lower(rawValue);
    if (isValidKind(c.kind))
        return;

    std::string ctx = lineNo > 0 ? std::format("{}:{}: ", filename.string(), lineNo) : "";

    for (auto v : validKinds) {
        if (is_typo(c.kind, v)) {
            cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}Invalid Kind '{}'. Did you mean {} ?", ctx, c.kind, v));
            return;
        }
    }
    cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}Invalid Kind '{}'", ctx, c.kind));
}

void Parser::setJobs(Config &c, const std::string &rawValue, std::size_t lineNo) {
    try {
        c.jobs = std::stoul(rawValue);
    } catch (const std::invalid_argument &) {
        cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: '{}' is not a valid number for 'jobs'", filename.string(), lineNo, rawValue));
    } catch (const std::out_of_range &) {
        cfg.v_Errors.push_back(std::format("[HelpMake] Error: {}:{}: '{}' is out of range for 'jobs'", filename.string(), lineNo, rawValue));
    }
}