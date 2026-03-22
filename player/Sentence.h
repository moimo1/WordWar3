#ifndef UNTITLED1_SENTENCE_H
#define UNTITLED1_SENTENCE_H

#include <vector>
#include <string>
#include "../word/Word.h"

class Sentence {
private:
    std::vector<Word> words;

public:
    Sentence();

    void addWord(const Word& word);
    void clear(); 
    const std::vector<Word>& getWords() const; 

    int calculateTotalEffect(EffectType type) const;
    int getEffectWordCount(EffectType type) const;
    std::string toString() const;
    std::string getPatternSignature() const;
};

#endif //UNTITLED1_SENTENCE_H