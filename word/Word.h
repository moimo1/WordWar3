#ifndef UNTITLED1_WORD_H
#define UNTITLED1_WORD_H

#include <string>
#include "../combat/Effect.h"

enum class PartOfSpeech {
    Noun,
    Verb,
    Adjective,
    Adverb,
    Pronoun,
    Article,
    Preposition,
    Conjunction
};

class Word {
private:
    std::string text;
    PartOfSpeech pos;
    Effect effect; // We use Composition here: a Word HAS an Effect

public:
    Word(const std::string& text, PartOfSpeech pos, Effect effect);

    const std::string& getText() const;
    PartOfSpeech getPartOfSpeech() const;
    const Effect& getEffect() const;
};

#endif //UNTITLED1_WORD_H