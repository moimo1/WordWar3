#include "WordDictionary.h"
#include <fstream>
#include <iostream>
#include <random>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

WordDictionary::WordDictionary(const std::string& jsonFilePath) {
    // 1. Open the JSON file
    std::ifstream file(jsonFilePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open database file: " << jsonFilePath << "\n";
        return;
    }

    // 2. Parse it using the nlohmann library
    json dbJson;
    try {
        file >> dbJson;
    } catch(const json::parse_error& e) {
        std::cerr << "JSON Parsing Error: " << e.what() << "\n";
        return;
    }

    // 3. Populate our memory map
    for (auto& el : dbJson.items()) {
        PartOfSpeech pos = stringToPOS(el.key());
        
        // Loop through the String array defined under this POS in the JSON
        for(const std::string& wordStr : el.value()) {
            categorizedWords[pos].push_back(wordStr);
        }
    }
}

PartOfSpeech WordDictionary::stringToPOS(const std::string& posStr) const {
    if (posStr == "Noun") return PartOfSpeech::Noun;
    if (posStr == "Verb") return PartOfSpeech::Verb;
    if (posStr == "Adjective") return PartOfSpeech::Adjective;
    if (posStr == "Adverb") return PartOfSpeech::Adverb;
    if (posStr == "Article") return PartOfSpeech::Article;
    if (posStr == "Pronoun") return PartOfSpeech::Pronoun;
    if (posStr == "Preposition") return PartOfSpeech::Preposition;
    if (posStr == "Conjunction") return PartOfSpeech::Conjunction;
    return PartOfSpeech::Noun; // Default fallback
}

Effect WordDictionary::generateRandomEffect() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    std::uniform_int_distribution<> typeDist(0, 6);
    
    // Magnitudes range from 1 to 20
    std::uniform_int_distribution<> valDist(1, 20);

    EffectType chosenType = static_cast<EffectType>(typeDist(gen));
    
    // Nullify value if no effect is applied
    if (chosenType == EffectType::None) {
        return Effect(chosenType, 0);
    }
    
    if (chosenType == EffectType::Critical) {
        return Effect(chosenType, 1);
    }

    return Effect(chosenType, valDist(gen));
}

std::vector<Word> WordDictionary::getRandomWords(int count) const {
    if (count <= 0) return {};

    std::vector<std::pair<std::string, PartOfSpeech>> subjects;
    std::vector<std::pair<std::string, PartOfSpeech>> verbs;
    std::vector<std::pair<std::string, PartOfSpeech>> poolRest;

    // 1. Sort the database out into pools so we can guarantee core grammatical minimums
    for (const auto& [pos, strList] : categorizedWords) {
        for (const std::string& str : strList) {
            if (pos == PartOfSpeech::Noun || pos == PartOfSpeech::Pronoun) {
                subjects.emplace_back(str, pos);
            } else if (pos == PartOfSpeech::Verb) {
                verbs.emplace_back(str, pos);
            } else {
                poolRest.emplace_back(str, pos);
            }
        }
    }

    std::random_device rd;
    std::mt19937 gen(rd());

    std::shuffle(subjects.begin(), subjects.end(), gen);
    std::shuffle(verbs.begin(), verbs.end(), gen);

    std::vector<std::pair<std::string, PartOfSpeech>> sampledWords;
    
    // explicitly enforce at least 3 Subjects and 3 Verbs to guarantee playable hands
    int requiredSubjects = std::min(3, (int)subjects.size());
    int requiredVerbs = std::min(3, (int)verbs.size());

    for (int i = 0; i < requiredSubjects; ++i) sampledWords.push_back(subjects[i]);
    for (int i = 0; i < requiredVerbs; ++i) sampledWords.push_back(verbs[i]);

    // push the remaining unpicked core words back into the general pool
    for (size_t i = requiredSubjects; i < subjects.size(); ++i) poolRest.push_back(subjects[i]);
    for (size_t i = requiredVerbs; i < verbs.size(); ++i) poolRest.push_back(verbs[i]);

    std::shuffle(poolRest.begin(), poolRest.end(), gen);

    int remainingToPick = count - (requiredSubjects + requiredVerbs);
    int toPickFromRest = std::min(remainingToPick, (int)poolRest.size());

    for (int i = 0; i < toPickFromRest; ++i) {
        sampledWords.push_back(poolRest[i]);
    }

    std::shuffle(sampledWords.begin(), sampledWords.end(), gen);

    // apply random combat effect magnitudes mapped to the generated Words
    std::vector<Word> result;
    result.reserve(sampledWords.size());
    for (const auto& pair : sampledWords) {
        Effect rolledEffect = generateRandomEffect();
        result.emplace_back(pair.first, pair.second, rolledEffect);
    }

    return result;
}

int WordDictionary::getDatabaseSize() const {
    int total = 0;
    for (const auto& pair : categorizedWords) {
        total += pair.second.size();
    }
    return total;
}