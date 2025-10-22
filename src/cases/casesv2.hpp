#pragma once
#include <functional>
#include <vector>
#include <algorithm>
#include <cmath>

class CasesV2 {
private:
    int currentCase = 1;
    int lastCase = 0;
    int maxCasesInput = 1;
    int iMaxCaseInput = 1;
    std::vector<int> casesToRun;
    std::vector<int>::const_iterator caseIt = casesToRun.cend();

public:
    int getCurrentCase() const noexcept { return currentCase; }

    void setMaxCases(int v) noexcept {
        maxCasesInput = std::abs(v) + 1;
        iMaxCaseInput = v;
        currentCase = 1;
        lastCase = 0;
        caseIt = casesToRun.cend();
    }

    void setCasesToRun(const std::vector<int>& v) {
        std::vector<int> tmp;
        tmp.reserve(v.size());
        for (int x : v) {
            x = std::abs(x);
            if (x != 0) tmp.push_back(x);
        }
        std::sort(tmp.begin(), tmp.end());
        tmp.erase(std::unique(tmp.begin(), tmp.end()), tmp.end());
        casesToRun = std::move(tmp);

        caseIt = casesToRun.cbegin();

        if (casesToRun.empty()) return;

        if (iMaxCaseInput == -1) {
            currentCase = casesToRun.front();
            maxCasesInput = casesToRun.back() + 1;
        } else if (iMaxCaseInput < -1) {
            currentCase = casesToRun.front();
            maxCasesInput = currentCase + std::abs(iMaxCaseInput);
        }
    }

    void increment() {
        lastCase = currentCase;
        if (iMaxCaseInput == -1 && caseIt != casesToRun.cend()) {
            ++caseIt;
            if (caseIt != casesToRun.cend()) {
                currentCase = *caseIt;
                return;
            }
        }
        ++currentCase;
    }

    void betweenIncrements(const std::function<void(int)>& func) const {
        int start = (lastCase == 0) ? 1 : lastCase + 1;
        for (int i = start; i <= currentCase; ++i) func(i);
    }

    bool isEndOfCases() const noexcept {
        return currentCase >= maxCasesInput;
    }
    CasesV2() = default;
    ~CasesV2() = default;
};
