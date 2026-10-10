// Self-check for scoring.h:
//   g++ -std=c++17 -I/usr/include/cjson -o scoring_test scoring_test.cc -lcjson && ./scoring_test
#include "scoring.h"
#include <cassert>
#include <cmath>

static bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }

int main()
{
    const char *path = "scoring_test.json";
    FILE *file = fopen(path, "w");
    fputs("{\"groups\": ["
          "{\"name\": \"uno\", \"points\": 10, \"tests\": \"s1_*\"},"
          "{\"points\": 20, \"type\": \"sum\", \"tests\": [\"s2_[12]\", \"s2_[345]\"]},"
          "{\"points\": 30, \"type\": \"min\", \"tests\": \"s3_*\"},"
          "{\"points\": 40, \"type\": \"mul\", \"tests\": \"s4_*\"},"
          "{\"points\": 5, \"tests\": \"empty_*\"},"
          "{\"points\": \"x\", \"tests\": \"s1_*\"}, {\"points\": 7}]}",
          file);
    fclose(file);
    std::vector<ScoreGroup> groups = read_score_groups(path);
    remove(path);
    assert(groups.size() == 5);
    assert(groups[0].type == "sum" && groups[0].patterns[0] == "s1_*" && groups[1].patterns.size() == 2);
    assert(read_score_groups("missing.json").empty());

    std::vector<TestOutcome> tests = {
        {"sample", 1},
        {"s1_1", 1}, {"s1_2", 1}, {"s1_3", 1}, {"s1_4", 1},  // 4 of 4
        {"s2_1", 1}, {"s2_2", 1}, {"s2_3", 0}, {"s2_4", 0}, {"s2_5", 0},  // 2 of 5
        {"s3_1", 1}, {"s3_2", 0},  // one failure voids the group
        {"s4_1", 0.5}, {"s4_2", 0.5},  // checker partials multiply
    };
    score_groups(groups, tests);
    assert(near(groups[0].earned, 10) && groups[0].tests == 4);
    assert(near(groups[1].earned, 8) && groups[1].tests == 5);
    assert(near(groups[2].earned, 0));
    assert(near(groups[3].earned, 10));
    assert(near(groups[4].earned, 0) && groups[4].tests == 0);
    puts("OK");
    return 0;
}
