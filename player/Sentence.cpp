#include "Sentence.h"

Sentence::Sentence() {}

void Sentence::addWord(const Word& word) { words.push_back(word); }

void Sentence::clear() { words.clear(); }

const std::vector<Word>& Sentence::getWords() const { return words; }

int Sentence::calculateTotalEffect(EffectType type) const {
    int total = 0;
    for (const auto& w : words) {
        if (w.getEffect().getType() == type) {
            total += w.getEffect().getValue();
        }
    }
    return total;
}

int Sentence::getEffectWordCount(EffectType type) const {
    int count = 0;
    for (const auto& w : words) {
        if (w.getEffect().getType() == type) {
            count++;
        }
    }
    return count;
}

std::string Sentence::toString() const {
    std::string result = "";
    for (size_t i = 0; i < words.size(); ++i) {
        result += words[i].getText();
        if (i < words.size() - 1) result += " ";
    }
    if (!result.empty()) result += ".";
    return result;
}

std::string Sentence::getPatternSignature() const {
    std::string sig = "";
    for (const auto& w : words) {
        switch(w.getPartOfSpeech()) {
            case PartOfSpeech::Noun: sig += "N"; break;
            case PartOfSpeech::Verb: sig += "V"; break;
            case PartOfSpeech::Adjective: sig += "J"; break;
            case PartOfSpeech::Adverb: sig += "D"; break;
            case PartOfSpeech::Article: sig += "A"; break;
            case PartOfSpeech::Pronoun: sig += "P"; break;
            case PartOfSpeech::Preposition: sig += "R"; break;
            case PartOfSpeech::Conjunction: sig += "C"; break;
        }
    }
    return sig;
}