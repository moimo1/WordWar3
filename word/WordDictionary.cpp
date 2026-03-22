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
    // Standard secure C++ random generator setup
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // We have 6 specific effect tags plus 'None' (Indices 0 through 6)
    std::uniform_int_distribution<> typeDist(0, 6);
    
    // Magnitudes range from 1 to 20
    std::uniform_int_distribution<> valDist(1, 20);

    EffectType chosenType = static_cast<EffectType>(typeDist(gen));
    
    // Nullify value if no effect is applied
    if (chosenType == EffectType::None) {
        return Effect(chosenType, 0);
    }
    
    // Critical usually acts as a binary multiplier flag, magnitude 1 is sufficient
    if (chosenType == EffectType::Critical) {
        return Effect(chosenType, 1);
    }

    return Effect(chosenType, valDist(gen));
}

std::vector<Word> WordDictionary::getRandomWords(int count) const {
    if (count <= 0) return {};

    // 1. Flatten all available words into a single lightweight vector
    std::vector<std::pair<std::string, PartOfSpeech>> allWords;
    for (const auto& [pos, strList] : categorizedWords) {
        for (const std::string& str : strList) {
            allWords.emplace_back(str, pos);
        }
    }

    if (allWords.empty()) return {};

    // 2. Cap the requested count
    if (count > static_cast<int>(allWords.size())) {
        count = allWords.size();
    }

    // 3. Shuffle our pool to randomize selection
    std::vector<std::pair<std::string, PartOfSpeech>> sampledWords;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::sample(allWords.begin(), allWords.end(), std::back_inserter(sampledWords), count, gen);

    // 4. Construct final Word objects with randomly assigned combat effects
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