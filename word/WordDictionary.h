#ifndef UNTITLED1_WORDDICTIONARY_H
#define UNTITLED1_WORDDICTIONARY_H

#include <vector>
#include <string>
#include <map>
#include "Word.h"

// Database class that reads from a JSON file and dynamically rolls randomized effects
class WordDictionary {
private:
    // We group raw words by their PartOfSpeech according to the JSON format.
    std::map<PartOfSpeech, std::vector<std::string>> categorizedWords;

    // A simple mapper to convert JSON string keys ("Noun") into our C++ enums (PartOfSpeech::Noun)
    PartOfSpeech stringToPOS(const std::string& posStr) const;
    
    // Core game mechanic: generate a random effect tag and random magnitude (1 to 20)
    Effect generateRandomEffect() const;

public:
    // The constructor reads the JSON file provided by the user.
    WordDictionary(const std::string& jsonFilePath);

    // Creates active Word objects pooling strings from the JSON and applying random Effects.
    std::vector<Word> getRandomWords(int count) const;
};

#endif //UNTITLED1_WORDDICTIONARY_H