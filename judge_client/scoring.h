// Group scoring (subtasks) for problems that declare data/<pid>/scoring.txt.
//
// Each line of the file is "<points> [sum|min|mul] <pattern>"; '#' starts a comment.
// The pattern is a glob over the test name without ".in"; a test may belong to several groups.
//   sum  points x average outcome: proportional to the tests passed (default)
//   min  points x worst outcome:   all or nothing with yes/no tests (CMS GroupMin)
//   mul  points x product:         CMS GroupMul
// A test outcome is 1 (passed), 0 (failed) or a fraction reported by the checker.
#ifndef SCORING_H
#define SCORING_H

#include <fnmatch.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

struct ScoreGroup
{
    double points;
    std::string type;
    std::string pattern;
    int tests;
    double earned;
};

struct TestOutcome
{
    std::string name;
    double outcome;
};

inline std::vector<ScoreGroup> read_score_groups(const char *path)
{
    std::vector<ScoreGroup> groups;
    FILE *file = fopen(path, "r");
    if (file == NULL)
        return groups;
    char line[1024];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *comment = strchr(line, '#');
        if (comment != NULL)
            *comment = 0;
        double points;
        char second[256] = "", third[256] = "";
        int fields = sscanf(line, "%lf %255s %255s", &points, second, third);
        if (fields < 2 || points < 0)
            continue;
        bool typed = fields == 3 && (!strcmp(second, "sum") || !strcmp(second, "min") || !strcmp(second, "mul"));
        groups.push_back({points, typed ? second : "sum", typed ? third : second, 0, 0});
    }
    fclose(file);
    return groups;
}

// Fills tests and earned of every group. A group without tests earns nothing.
inline void score_groups(std::vector<ScoreGroup> &groups, const std::vector<TestOutcome> &tests)
{
    for (ScoreGroup &group : groups)
    {
        double sum = 0, lowest = 1, product = 1;
        group.tests = 0;
        for (const TestOutcome &test : tests)
        {
            if (fnmatch(group.pattern.c_str(), test.name.c_str(), 0) != 0)
                continue;
            group.tests++;
            sum += test.outcome;
            lowest = std::min(lowest, test.outcome);
            product *= test.outcome;
        }
        double factor = group.tests == 0 ? 0 : group.type == "min" ? lowest : group.type == "mul" ? product : sum / group.tests;
        group.earned = group.points * factor;
    }
}

#endif
