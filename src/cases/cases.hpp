#pragma once
#include <functional>
#include <vector>
#include <algorithm>
#include <cmath>
class Cases {
private:
    int currentCase = 1;
    int lastCase = 0;
    int maxCasesInput = 1;
    int iMaxCaseInput = 1;
    std::size_t caseIndex = 0;
    std::vector<int> casesToRun;
public:
    int getCurrentCase() const noexcept {
        return this->currentCase;
    }
    void setMaxCases(const int v) {
        this->maxCasesInput = std::abs(v) + 1;
        this->iMaxCaseInput = v;
        this->currentCase = 1;
        this->lastCase = 0;
        this->caseIndex = 0;
    }
    void setCasesToRun(const std::vector<int> &v) {
        std::vector<int> tmp;
        tmp.reserve(v.size());
        for (int x : v) {
            x = std::abs(x);
            if (x != 0) tmp.push_back(x);
        }
        std::sort(tmp.begin(),tmp.end());
        tmp.erase(std::unique(tmp.begin(), tmp.end()), tmp.end());

        this->casesToRun = std::move(tmp);
        this->caseIndex = 0;

        if (this->casesToRun.empty()) return;
        if (this->iMaxCaseInput == -1) {
            this->currentCase = this->casesToRun.front();
            this->maxCasesInput = this->casesToRun.back() + 1;
        }
        if (this->iMaxCaseInput < -1) {
            this->currentCase = this->casesToRun.front();
            this->maxCasesInput = this->currentCase + std::abs(this->iMaxCaseInput);

        }
    }
    void increment() {
        if (this->iMaxCaseInput == -1 && this->casesToRun.size()) {
            if ((this->caseIndex + 1) < this->casesToRun.size()) {
                this->lastCase = this->currentCase;
                ++this->caseIndex;
                this->currentCase = this->casesToRun.at(this->caseIndex);
            } else {
                this->lastCase = this->currentCase;
                ++this->currentCase;
            }
        } else {
            this->lastCase = this->currentCase;
            ++this->currentCase;
        }
    }
    void betweenIncrements(const std::function<void(const int)> &func) const {
        int lastcase = (this->lastCase == 0) ? 1 : this->lastCase + 1;
        for (int i = lastcase; i <= this->currentCase; ++i) func(i);
    }
    bool isEndOfCases() const noexcept {
        return this->currentCase >= this->maxCasesInput;
    }
    Cases() = default;
    ~Cases() = default;
};