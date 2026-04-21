#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <set>

using namespace std;

enum Status {
    Accepted,
    Wrong_Answer,
    Runtime_Error,
    Time_Limit_Exceed,
    Unknown
};

Status string_to_status(const string& s) {
    if (s == "Accepted") return Accepted;
    if (s == "Wrong_Answer") return Wrong_Answer;
    if (s == "Runtime_Error") return Runtime_Error;
    if (s == "Time_Limit_Exceed") return Time_Limit_Exceed;
    return Unknown;
}

string status_to_string(Status s) {
    if (s == Accepted) return "Accepted";
    if (s == Wrong_Answer) return "Wrong_Answer";
    if (s == Runtime_Error) return "Runtime_Error";
    if (s == Time_Limit_Exceed) return "Time_Limit_Exceed";
    return "Unknown";
}

struct Submission {
    char problem;
    Status status;
    int time;
};

struct ProblemStatus {
    bool solved = false;
    int first_ac_time = 0;
    int failed_before = 0;

    // For freezing
    bool frozen = false;
    int frozen_failed = 0; // failed attempts after freezing
    int frozen_total = 0; // total attempts after freezing
    bool frozen_has_ac = false;
    int frozen_ac_time = 0;
};

struct Team {
    string name;
    int solved_count = 0;
    long long penalty = 0;
    vector<int> ac_times; // Sorted descending for comparison
    ProblemStatus probs[26];

    // For scoreboard display (flushed state)
    int display_solved = 0;
    long long display_penalty = 0;
    vector<int> display_ac_times;
    int last_flushed_rank = 0;

    void update_display() {
        display_solved = 0;
        display_penalty = 0;
        display_ac_times.clear();
        for (int i = 0; i < 26; ++i) {
            if (probs[i].solved && !probs[i].frozen) {
                display_solved++;
                display_penalty += 20LL * probs[i].failed_before + probs[i].first_ac_time;
                display_ac_times.push_back(probs[i].first_ac_time);
            }
        }
        sort(display_ac_times.begin(), display_ac_times.end(), greater<int>());
    }

    bool operator<(const Team& other) const {
        if (display_solved != other.display_solved)
            return display_solved > other.display_solved;
        if (display_penalty != other.display_penalty)
            return display_penalty < other.display_penalty;

        // Compare max AC time, then second max, etc.
        // Rule: smaller solve time ranks higher
        for (size_t i = 0; i < display_ac_times.size(); ++i) {
            if (display_ac_times[i] != other.display_ac_times[i])
                return display_ac_times[i] < other.display_ac_times[i];
        }

        return name < other.name;
    }

    static bool initial_cmp(const Team* a, const Team* b) {
        return a->name < b->name;
    }
};

map<string, Team> teams_map;
vector<string> team_names;
vector<Team*> scoreboard;
vector<Submission> submissions_all[10005]; // Index by team name index
map<string, int> name_to_idx;
int duration = 0, problem_count = 0;
bool started = false, frozen = false;

void flush_scoreboard() {
    for (auto t : scoreboard) {
        t->update_display();
    }
    stable_sort(scoreboard.begin(), scoreboard.end(), [](Team* a, Team* b) {
        return (*a) < (*b);
    });
    for (int i = 0; i < (int)scoreboard.size(); ++i) {
        scoreboard[i]->last_flushed_rank = i + 1;
    }
}

void print_scoreboard() {
    for (int i = 0; i < scoreboard.size(); ++i) {
        Team& t = *scoreboard[i];
        cout << t.name << " " << (i + 1) << " " << t.display_solved << " " << t.display_penalty;
        for (int j = 0; j < problem_count; ++j) {
            cout << " ";
            if (t.probs[j].frozen) {
                // Frozen: -x/y or 0/y
                int x = t.probs[j].failed_before;
                int y = t.probs[j].frozen_total;
                if (x == 0) cout << "0/" << y;
                else cout << "-" << x << "/" << y;
            } else if (t.probs[j].solved) {
                // Solved: + or +x
                if (t.probs[j].failed_before == 0) cout << "+";
                else cout << "+" << t.probs[j].failed_before;
            } else {
                // Not solved: . or -x
                if (t.probs[j].failed_before == 0) cout << ".";
                else cout << "-" << t.probs[j].failed_before;
            }
        }
        cout << "\n";
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(NULL);

    string cmd;
    while (cin >> cmd) {
        if (cmd == "ADDTEAM") {
            string name;
            cin >> name;
            if (started) {
                cout << "[Error]Add failed: competition has started.\n";
            } else if (teams_map.count(name)) {
                cout << "[Error]Add failed: duplicated team name.\n";
            } else {
                teams_map[name] = {name};
                cout << "[Info]Add successfully.\n";
            }
        } else if (cmd == "START") {
            string dummy;
            cin >> dummy >> duration >> dummy >> problem_count;
            if (started) {
                cout << "[Error]Start failed: competition has started.\n";
            } else {
                started = true;
                team_names.clear();
                for (auto const& [name, team] : teams_map) {
                    team_names.push_back(name);
                }
                sort(team_names.begin(), team_names.end());
                for (int i = 0; i < (int)team_names.size(); ++i) {
                    name_to_idx[team_names[i]] = i;
                    scoreboard.push_back(&teams_map[team_names[i]]);
                    scoreboard.back()->last_flushed_rank = i + 1;
                }
                cout << "[Info]Competition starts.\n";
            }
        } else if (cmd == "SUBMIT") {
            string prob_s, dummy, team_name, status_s, time_s;
            int time;
            cin >> prob_s >> dummy >> team_name >> dummy >> status_s >> dummy >> time_s;
            time = stoi(time_s);
            char prob_id = prob_s[0];
            int p_idx = prob_id - 'A';
            Status status = string_to_status(status_s);

            int t_idx = name_to_idx[team_name];
            submissions_all[t_idx].push_back({prob_id, status, time});

            Team& t = teams_map[team_name];
            if (!t.probs[p_idx].solved) {
                if (frozen) {
                    // Problems solved before freezing will not be frozen even if submitted again after freezing
                    // So if it's not solved, it becomes frozen.
                    t.probs[p_idx].frozen = true;
                    t.probs[p_idx].frozen_total++;
                    if (status == Accepted) {
                        if (!t.probs[p_idx].frozen_has_ac) {
                            t.probs[p_idx].frozen_has_ac = true;
                            t.probs[p_idx].frozen_ac_time = time;
                        }
                    } else {
                        if (!t.probs[p_idx].frozen_has_ac) {
                            t.probs[p_idx].frozen_failed++;
                        }
                    }
                } else {
                    if (status == Accepted) {
                        t.probs[p_idx].solved = true;
                        t.probs[p_idx].first_ac_time = time;
                        t.probs[p_idx].frozen = false; // Just in case
                        t.solved_count++;
                        t.penalty += 20LL * t.probs[p_idx].failed_before + time;
                        t.ac_times.push_back(time);
                        sort(t.ac_times.rbegin(), t.ac_times.rend());
                    } else {
                        t.probs[p_idx].failed_before++;
                    }
                }
            }
        } else if (cmd == "FLUSH") {
            flush_scoreboard();
            cout << "[Info]Flush scoreboard.\n";
        } else if (cmd == "FREEZE") {
            if (frozen) {
                cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            } else {
                frozen = true;
                cout << "[Info]Freeze scoreboard.\n";
            }
        } else if (cmd == "SCROLL") {
            if (!frozen) {
                cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            } else {
                flush_scoreboard();
                cout << "[Info]Scroll scoreboard.\n";
                print_scoreboard();

                while (true) {
                    int target_team_idx = -1;
                    int target_prob_idx = -1;

                    for (int i = scoreboard.size() - 1; i >= 0; --i) {
                        for (int j = 0; j < problem_count; ++j) {
                            if (scoreboard[i]->probs[j].frozen) {
                                target_team_idx = i;
                                target_prob_idx = j;
                                goto found;
                            }
                        }
                    }
                    found:
                    if (target_team_idx == -1) break;

                    Team& t = *scoreboard[target_team_idx];
                    int old_idx = target_team_idx;
                    t.probs[target_prob_idx].frozen = false;
                    if (t.probs[target_prob_idx].frozen_has_ac) {
                        t.probs[target_prob_idx].solved = true;
                        t.probs[target_prob_idx].first_ac_time = t.probs[target_prob_idx].frozen_ac_time;
                        t.probs[target_prob_idx].failed_before += t.probs[target_prob_idx].frozen_failed;

                        // Update display stats for ranking comparison
                        t.update_display();

                        // Move team up in scoreboard
                        int cur_pos = old_idx;
                        while (cur_pos > 0 && (*scoreboard[cur_pos]) < (*scoreboard[cur_pos - 1])) {
                            Team* moving_team = scoreboard[cur_pos];
                            Team* higher_team = scoreboard[cur_pos - 1];
                            swap(scoreboard[cur_pos], scoreboard[cur_pos - 1]);
                            cur_pos--;
                            cout << moving_team->name << " " << higher_team->name << " " << moving_team->display_solved << " " << moving_team->display_penalty << "\n";
                        }
                    } else {
                        t.probs[target_prob_idx].failed_before += t.probs[target_prob_idx].frozen_total;
                        t.update_display();
                        // No ranking change possible when only adding failed attempts
                    }
                }

                print_scoreboard();
                // Update last_flushed_rank
                for (int i = 0; i < scoreboard.size(); ++i) {
                    scoreboard[i]->last_flushed_rank = i + 1;
                }
                frozen = false;
            }
        } else if (cmd == "QUERY_RANKING") {
            string name;
            cin >> name;
            if (!teams_map.count(name)) {
                cout << "[Error]Query ranking failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query ranking.\n";
                if (frozen) {
                    cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
                }
                cout << name << " NOW AT RANKING " << teams_map[name].last_flushed_rank << "\n";
            }
        } else if (cmd == "QUERY_SUBMISSION") {
            string team_name, dummy, prob_cond, status_cond;
            cin >> team_name >> dummy >> prob_cond >> dummy >> status_cond;
            // Parse PROBLEM=ALL or PROBLEM=A
            string p_val = prob_cond.substr(8);
            // Parse STATUS=ALL or STATUS=Accepted
            string s_val = status_cond.substr(7);

            if (!teams_map.count(team_name)) {
                cout << "[Error]Query submission failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query submission.\n";
                int t_idx = name_to_idx[team_name];
                bool found_sub = false;
                for (int i = (int)submissions_all[t_idx].size() - 1; i >= 0; --i) {
                    const auto& sub = submissions_all[t_idx][i];
                    bool p_match = (p_val == "ALL" || p_val[0] == sub.problem);
                    bool s_match = (s_val == "ALL" || s_val == status_to_string(sub.status));
                    if (p_match && s_match) {
                        cout << team_name << " " << sub.problem << " " << status_to_string(sub.status) << " " << sub.time << "\n";
                        found_sub = true;
                        break;
                    }
                }
                if (!found_sub) {
                    cout << "Cannot find any submission.\n";
                }
            }
        } else if (cmd == "END") {
            cout << "[Info]Competition ends.\n";
            break;
        }
    }

    return 0;
}
