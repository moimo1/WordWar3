#ifndef UNTITLED1_WORD_H
#define UNTITLED1_WORD_H

#include <string>
#include <nlohmann/json.hpp>
#include "../combat/Effect.h"

enum class PartOfSpeech {
    Noun, Verb, Adjective, Adverb, Pronoun, Article, Preposition, Conjunction
};

class Word {
private:
    std::string text;
    PartOfSpeech pos;
    Effect effect;

public:
    Word(const std::string& text, PartOfSpeech pos, Effect effect);

    const std::string& getText() const;
    PartOfSpeech getPartOfSpeech() const;
    const Effect& getEffect() const;

    // Networking serialization capabilities
    nlohmann::json toJson() const;
    static Word fromJson(const nlohmann::json& j);
};

#endif //UNTITLED1_WORD_H