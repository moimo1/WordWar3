#ifndef UNTITLED1_SENTENCE_H
#define UNTITLED1_SENTENCE_H

#include <vector>
#include <string>
#include "../word/Word.h"
#include "../combat/Effect.h"

class Sentence {
private:
    std::vector<Word> words;

public:
    Sentence();

    void addWord(const Word& word);
    int calculateTotalEffect(EffectType type) const;
    std::string toString() const;
};

#endif //UNTITLED1_SENTENCE_H