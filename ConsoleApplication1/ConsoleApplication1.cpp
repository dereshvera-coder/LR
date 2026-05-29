#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <climits>
#include <cctype>
#include <numeric>

using namespace std;


void skip_whitespace(const string& s, size_t& i) {
    while (i < s.size() && isspace(static_cast<unsigned char>(s[i]))) ++i;
}

int read_number(const string& s, size_t& i) {
    skip_whitespace(s, i);
    int sign = 1;
    if (s[i] == '-') { sign = -1; ++i; }
    int val = 0;
    while (i < s.size() && isdigit(static_cast<unsigned char>(s[i]))) {
        val = val * 10 + (s[i] - '0');
        ++i;
    }
    return val * sign;
}

string read_string(const string& s, size_t& i) {
    skip_whitespace(s, i);
    if (s[i] != '"') throw runtime_error("Expected '\"'");
    ++i;
    string res;
    while (i < s.size() && s[i] != '"') {
        if (s[i] == '\\') {
            ++i;
            if (i < s.size()) res += s[i];
            ++i;
        }
        else {
            res += s[i];
            ++i;
        }
    }
    if (i >= s.size() || s[i] != '"') throw runtime_error("Unterminated string");
    ++i;
    return res;
}

vector<pair<int, int>> parse_wallet(const string& s, size_t& i) {
    skip_whitespace(s, i);
    if (s[i] != '[') throw runtime_error("Expected '[' for wallet");
    ++i;
    vector<pair<int, int>> wallet;
    while (true) {
        skip_whitespace(s, i);
        if (s[i] == ']') { ++i; break; }
        if (s[i] != '[') throw runtime_error("Expected '[' for inner array");
        ++i;
        int nominal = read_number(s, i);
        skip_whitespace(s, i);
        if (s[i] != ',') throw runtime_error("Expected ',' after nominal");
        ++i;
        int count = read_number(s, i);
        skip_whitespace(s, i);
        if (s[i] != ']') throw runtime_error("Expected ']' after count");
        ++i;
        wallet.push_back({ nominal, count });
        skip_whitespace(s, i);
        if (s[i] == ',') {
            ++i;
            continue;
        }
        else if (s[i] == ']') {
            ++i;
            break;
        }
        else {
            throw runtime_error("Expected ',' or ']' after inner array");
        }
    }
    return wallet;
}

struct TestCase {
    vector<pair<int, int>> wallet;
    int amount;
    string strategy;
};

vector<TestCase> parse_tests(const string& s) {
    size_t i = 0;
    skip_whitespace(s, i);
    if (s[i] != '[') throw runtime_error("Expected '[' at top level");
    ++i;
    vector<TestCase> tests;
    while (true) {
        skip_whitespace(s, i);
        if (s[i] == ']') { ++i; break; }
        if (s[i] != '{') throw runtime_error("Expected '{'");
        ++i;
        TestCase tc;
        while (true) {
            skip_whitespace(s, i);
            if (s[i] == '}') { ++i; break; }
            string key = read_string(s, i);
            skip_whitespace(s, i);
            if (s[i] != ':') throw runtime_error("Expected ':'");
            ++i;
            if (key == "wallet") {
                tc.wallet = parse_wallet(s, i);
            }
            else if (key == "amount") {
                tc.amount = read_number(s, i);
            }
            else if (key == "strategy") {
                tc.strategy = read_string(s, i);
            }
            else {
                throw runtime_error("Unknown key: " + key);
            }
            skip_whitespace(s, i);
            if (s[i] == ',') {
                ++i;
                continue;
            }
            else if (s[i] == '}') {
                continue;
            }
            else {
                throw runtime_error("Expected ',' or '}'");
            }
        }
        tests.push_back(tc);
        skip_whitespace(s, i);
        if (s[i] == ',') {
            ++i;
            continue;
        }
        else if (s[i] == ']') {
            continue;
        }
        else {
            throw runtime_error("Expected ',' or ']'");
        }
    }
    return tests;
}


vector<pair<int, int>> solve_max(const vector<int>& denom, const vector<int>& avail, int amount) {
    int n = denom.size();
    vector<int> idx(n);
    iota(idx.begin(), idx.end(), 0);
    sort(idx.begin(), idx.end(), [&](int a, int b) { return denom[a] > denom[b]; });
    vector<int> counts(n, 0);
    function<bool(int, int)> backtrack = [&](int pos, int rem) -> bool {
        if (pos == n) return rem == 0;
        int i = idx[pos];
        int max_cnt = min(avail[i], rem / denom[i]);
        long long max_future = 0;
        for (int j = pos + 1; j < n; ++j) {
            int jj = idx[j];
            max_future += (long long)denom[jj] * avail[jj];
        }
        int min_denom = INT_MAX;
        for (int j = pos + 1; j < n; ++j) {
            min_denom = min(min_denom, denom[idx[j]]);
        }
        for (int cnt = max_cnt; cnt >= 0; --cnt) {
            int new_rem = rem - cnt * denom[i];
            if (new_rem < 0) continue;
            if (new_rem > max_future) continue;
            if (new_rem > 0 && new_rem < min_denom) continue;
            counts[i] = cnt;
            if (backtrack(pos + 1, new_rem)) return true;
        }
        counts[i] = 0;
        return false;
        };
    if (!backtrack(0, amount)) return {};
    vector<pair<int, int>> res;
    for (int i = 0; i < n; ++i) {
        if (counts[i] > 0) res.push_back({ denom[i], counts[i] });
    }
    return res;
}

vector<pair<int, int>> solve_min(const vector<int>& denom, const vector<int>& avail, int amount) {
    int n = denom.size();
    vector<int> idx(n);
    iota(idx.begin(), idx.end(), 0);
    sort(idx.begin(), idx.end(), [&](int a, int b) { return denom[a] < denom[b]; });
    vector<int> counts(n, 0);
    function<bool(int, int)> backtrack = [&](int pos, int rem) -> bool {
        if (pos == n) return rem == 0;
        int i = idx[pos];
        int max_cnt = min(avail[i], rem / denom[i]);
        long long max_future = 0;
        for (int j = pos + 1; j < n; ++j) {
            int jj = idx[j];
            max_future += (long long)denom[jj] * avail[jj];
        }
        int min_denom = INT_MAX;
        for (int j = pos + 1; j < n; ++j) {
            min_denom = min(min_denom, denom[idx[j]]);
        }
        for (int cnt = max_cnt; cnt >= 0; --cnt) {
            int new_rem = rem - cnt * denom[i];
            if (new_rem < 0) continue;
            if (new_rem > max_future) continue;
            if (new_rem > 0 && new_rem < min_denom) continue;
            counts[i] = cnt;
            if (backtrack(pos + 1, new_rem)) return true;
        }
        counts[i] = 0;
        return false;
        };
    if (!backtrack(0, amount)) return {};
    vector<pair<int, int>> res;
    for (int i = 0; i < n; ++i) {
        if (counts[i] > 0) res.push_back({ denom[i], counts[i] });
    }
    return res;
}

vector<pair<int, int>> solve_uniform(const vector<int>& denom, const vector<int>& avail, int amount) {
    int n = denom.size();
    vector<int> any_counts(n, 0);
    bool any_found = false;
    {
        vector<int> idx(n);
        iota(idx.begin(), idx.end(), 0);
        sort(idx.begin(), idx.end(), [&](int a, int b) { return denom[a] > denom[b]; });
        function<bool(int, int)> find_any = [&](int pos, int rem) -> bool {
            if (pos == n) return rem == 0;
            int i = idx[pos];
            int max_cnt = min(avail[i], rem / denom[i]);
            for (int cnt = max_cnt; cnt >= 0; --cnt) {
                int new_rem = rem - cnt * denom[i];
                if (new_rem < 0) continue;
                any_counts[i] = cnt;
                if (find_any(pos + 1, new_rem)) return true;
            }
            any_counts[i] = 0;
            return false;
            };
        any_found = find_any(0, amount);
    }
    if (!any_found) return {};


    int best_diff = *max_element(any_counts.begin(), any_counts.end()) -
        *min_element(any_counts.begin(), any_counts.end());
    vector<int> best_counts = any_counts;


    vector<int> order(n);
    iota(order.begin(), order.end(), 0);
    sort(order.begin(), order.end(), [&](int a, int b) { return denom[a] < denom[b]; });

    vector<long long> max_suffix_sum(n + 1, 0);
    vector<int> min_denom_suffix(n + 1, INT_MAX);
    for (int i = n - 1; i >= 0; --i) {
        int d = denom[order[i]];
        int a = avail[order[i]];
        max_suffix_sum[i] = max_suffix_sum[i + 1] + (long long)d * a;
        min_denom_suffix[i] = min(d, min_denom_suffix[i + 1]);
    }

    vector<int> cur_counts(n, 0);

    function<void(int, int, int, int, int)> dfs = [&](int pos, int rem, int cur_min, int cur_max, int cur_diff) {
        if (cur_diff >= best_diff) return;
        if (pos == n) {
            if (rem == 0) {
                best_diff = cur_diff;
                best_counts = cur_counts;
            }
            return;
        }
        int i = order[pos];
        int max_cnt = min(avail[i], rem / denom[i]);

        int low = 0, high = max_cnt;
        if (cur_min != INT_MAX) {
            if (cur_max - cur_min >= best_diff) return;
            low = max(low, cur_max - best_diff + 1);
            high = min(high, cur_min + best_diff - 1);
        }
        if (low > high) return;

        for (int cnt = low; cnt <= high; ++cnt) {
            int new_rem = rem - cnt * denom[i];
            if (new_rem < 0) continue;
            if (new_rem > max_suffix_sum[pos + 1]) continue;
            if (new_rem > 0 && new_rem < min_denom_suffix[pos + 1]) continue;

            int new_min = cur_min, new_max = cur_max;
            if (cur_min == INT_MAX) {
                new_min = new_max = cnt;
            }
            else {
                if (cnt < new_min) new_min = cnt;
                if (cnt > new_max) new_max = cnt;
            }
            int new_diff = new_max - new_min;
            if (new_diff >= best_diff) continue;

            cur_counts[i] = cnt;
            dfs(pos + 1, new_rem, new_min, new_max, new_diff);
        }
        };

    dfs(0, amount, INT_MAX, INT_MIN, 0);

    vector<pair<int, int>> res;
    for (int i = 0; i < n; ++i) {
        if (best_counts[i] > 0) res.push_back({ denom[i], best_counts[i] });
    }
    return res;
}

vector<pair<int, int>> solve_test(const TestCase& tc) {
    int n = tc.wallet.size();
    vector<int> denom(n), avail(n);
    for (int i = 0; i < n; ++i) {
        denom[i] = tc.wallet[i].first;
        avail[i] = tc.wallet[i].second;
    }
    if (tc.strategy == "MAX") {
        return solve_max(denom, avail, tc.amount);
    }
    else if (tc.strategy == "MIN") {
        return solve_min(denom, avail, tc.amount);
    }
    else if (tc.strategy == "UNIFORM") {
        return solve_uniform(denom, avail, tc.amount);
    }
    else {
        return {};
    }
}


string to_json(const vector<vector<pair<int, int>>>& results) {
    string out = "[";
    for (size_t i = 0; i < results.size(); ++i) {
        if (i > 0) out += ",";
        out += "{\"dispense\":[";
        const auto& disp = results[i];
        for (size_t j = 0; j < disp.size(); ++j) {
            if (j > 0) out += ",";
            out += "[" + to_string(disp[j].first) + "," + to_string(disp[j].second) + "]";
        }
        out += "]}";
    }
    out += "]";
    return out;
}


int main() {
    try {
        ifstream in("input.json");
        if (!in.is_open()) {
            cerr << "Cannot open input.json" << endl;
            return 1;
        }
        string content((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        in.close();

        vector<TestCase> tests = parse_tests(content);
        vector<vector<pair<int, int>>> results;
        for (const auto& test : tests) {
            results.push_back(solve_test(test));
        }

        ofstream out("output.json");
        if (!out.is_open()) {
            cerr << "Cannot open output.json" << endl;
            return 1;
        }
        out << to_json(results);
        out.close();
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}



