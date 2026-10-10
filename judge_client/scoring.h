// Group scoring (subtasks) for problems that declare data/<pid>/scoring.json:
//
//   { "groups": [ { "name": "Subtarea 1", "points": 10, "type": "sum", "tests": "s1_*" },
//                 { "points": 20, "type": "min", "tests": ["s2_*", "extra"] } ] }
//
// "tests" is a glob (or a list of them) over the test name without ".in"; a test may belong to
// several groups. "type" is optional:
//   sum  points x average outcome: proportional to the tests passed (default)
//   min  points x worst outcome:   all or nothing with yes/no tests (CMS GroupMin)
//   mul  points x product:         CMS GroupMul
// A test outcome is 1 (passed), 0 (failed) or a fraction reported by the checker.
#ifndef SCORING_H
#define SCORING_H

#include <fnmatch.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "cJSON.h"

struct ScoreGroup
{
    double points;
    std::string type;
    std::vector<std::string> patterns;
    int tests;
    double earned;
};

struct TestOutcome
{
    std::string name;
    double outcome;
};

// Groups that are malformed (no points, no tests) are skipped; an unreadable file gives none.
inline std::vector<ScoreGroup> read_score_groups(const char *path)
{
    std::vector<ScoreGroup> groups;
    FILE *file = fopen(path, "rb");
    if (file == NULL)
        return groups;
    std::string text;
    char buffer[4096];
    size_t read;
    while ((read = fread(buffer, 1, sizeof(buffer), file)) > 0 && text.size() < (1 << 20))
        text.append(buffer, read);
    fclose(file);

    cJSON *root = cJSON_Parse(text.c_str());
    cJSON *list = cJSON_GetObjectItem(root, "groups");
    cJSON *item;
    cJSON_ArrayForEach(item, list)
    {
        cJSON *points = cJSON_GetObjectItem(item, "points");
        cJSON *type = cJSON_GetObjectItem(item, "type");
        cJSON *tests = cJSON_GetObjectItem(item, "tests");
        if (!cJSON_IsNumber(points) || points->valuedouble < 0)
            continue;
        ScoreGroup group = {points->valuedouble, "sum", {}, 0, 0};
        if (cJSON_IsString(type) && (std::string(type->valuestring) == "min" || std::string(type->valuestring) == "mul"))
            group.type = type->valuestring;
        if (cJSON_IsString(tests))
            group.patterns.push_back(tests->valuestring);
        cJSON *pattern, *list_of_patterns = cJSON_IsArray(tests) ? tests : NULL;
        cJSON_ArrayForEach(pattern, list_of_patterns)
        {
            if (cJSON_IsString(pattern))
                group.patterns.push_back(pattern->valuestring);
        }
        if (!group.patterns.empty())
            groups.push_back(group);
    }
    cJSON_Delete(root);
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
            bool member = false;
            for (const std::string &pattern : group.patterns)
                member = member || fnmatch(pattern.c_str(), test.name.c_str(), 0) == 0;
            if (!member)
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
