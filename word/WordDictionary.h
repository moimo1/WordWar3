#ifndef UNTITLED1_WORDDICTIONARY_H
#define UNTITLED1_WORDDICTIONARY_H

#include <vector>
#include <string>
#include <map>
#include "Word.h"

class WordDictionary {
private:
    std::map<PartOfSpeech, std::vector<std::string>> categorizedWords;
    PartOfSpeech stringToPOS(const std::string& posStr) const;
    Effect generateRandomEffect() const;

public:
    WordDictionary(const std::string& jsonFilePath);
    std::vector<Word> getRandomWords(int count) const;
    int getDatabaseSize() const;
};

#endif //UNTITLED1_WORDDICTIONARY_H