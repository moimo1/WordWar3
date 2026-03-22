#include "Sentence.h"

Sentence::Sentence() {
}

void Sentence::addWord(const Word& word) {
    words.push_back(word);
}

int Sentence::calculateTotalEffect(EffectType type) const {
    int total = 0;
    for (const auto& w : words) {
        if (w.getEffect().getType() == type) {
            total += w.getEffect().getValue();
        }
    }
    return total;
}

std::string Sentence::toString() const {
    std::string result = "";
    for (size_t i = 0; i < words.size(); ++i) {
        result += words[i].getText();
        if (i < words.size() - 1) {
            result += " ";
        }
    }
    if (!result.empty()) {
        result += ".";
    }
    return result;
}