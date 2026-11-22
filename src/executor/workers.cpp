#include "hm/parser/parser.hpp"
#include "hm/process/process.hpp"
#include "hp/other/other.hpp"

int Parser::runJobs(std::vector<Job> &jobs, std::size_t n) {
    if (jobs.empty())
        return 0;
    if (n == 0)
        n = std::thread::hardware_concurrency();
    if (n == 0)
        n = 1;
    n = (std::min<std::size_t>)(n, jobs.size());

    std::vector<int> results(n, 0);
    std::vector<double> times(jobs.size(), 0.0);
    std::vector<std::thread> pool;
    std::size_t chunk = (jobs.size() + n - 1) / n;

    for (std::size_t t = 0; t < n; ++t) {
        std::size_t begin = t * chunk;
        std::size_t end = (std::min)(begin + chunk, jobs.size());
        if (begin >= jobs.size())
            break;

        pool.emplace_back([&, t, begin, end]() {
            for (std::size_t i = begin; i < end; ++i) {
                auto &j = jobs[i];
                std::string command = hm::Process::join(j.args);
                if (command.size() > 8000 && j.responseArgStart < j.args.size()) {
                    std::filesystem::path rspPath(j.responseFilePath);

                    std::vector<std::string> responseArgs(j.args.begin() + static_cast<long long>(j.responseArgStart), j.args.end());
                    std::string rspArg = hm::Process::writeResponseFile(responseArgs, rspPath);
                    if (rspArg.empty()) {
                        hp::printlnCl(std::format("[HelpMake] Error: Could not write response file '{}'.\n", rspPath.string()), hp::RED);
                        results[t] = -1;
                        j.failed = true;
                        continue;
                    }

                    std::vector<std::string> commandPrefix(j.args.begin(), j.args.begin() + static_cast<long long>(j.responseArgStart));
                    command = hm::Process::join(commandPrefix) + " " + rspArg;

                    if (cfg.verbose || cfg.debug)
                        hp::printlnCl(std::format("[HelpMake] Command too long; using response file: {}", rspPath.string()), hp::YELLOW);
                }

                std::string redirect = std::format("{} > \"{}\"", command, j.logPath);
                auto timer = hp::startTimer();
                int rc = hm::Process::run(redirect).exitCode;
                double elapsed = hp::stopTimer(timer);

                times[i] = elapsed;

                if (rc != 0) {
                    results[t] = rc;
                    j.failed = true;

                    std::ifstream logFile(j.logPath);
                    if (logFile.is_open()) {
                        std::string content((std::istreambuf_iterator<char>(logFile)),
                                            std::istreambuf_iterator<char>());
                        if (!content.empty()) {
                            std::cerr << content;
                            if (content.back() != '\n')
                                std::cerr << '\n';
                        }
                    }
                    continue;
                }
            }
        });
    }

    for (auto &th : pool)
        th.join();

    if (!cfg.quiet) {
        hp::printlnCl(std::format("\n[HelpMake] Compile phase ({} job{}):", n, n == 0 ? "" : "s"), hp::YELLOW);
        for (std::size_t i = 0; i < jobs.size(); ++i) {
            if (jobs[i].failed)
                hp::printlnCl(std::format("- {} failed to compile", jobs[i].file), hp::RED);
            else
                hp::printlnCl(std::format("- {} compiled in {}s", jobs[i].file, times[i]), hp::YELLOW);
        }
    }

    for (int rc : results)
        if (rc != 0)
            return rc;

    return 0;
}