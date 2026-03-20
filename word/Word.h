//
// Created by ASUS TUF F15 on 3/18/2026.
//

#ifndef UNTITLED1_WORD_H
#define UNTITLED1_WORD_H

#include <string>

// 'enum class' provides strong typing (prevents implicit conversions to integers).
// This makes the code safer and easier to read.
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

// Represents the effect a word can trigger when used in a valid sentence.
enum class EffectTag {
    None,
    Offense,
    Defense,
    Critical,
    Heal
};

class Word {
private:
    std::string text;
    PartOfSpeech pos;
    EffectTag effect;
    int effectValue;

public:
    Word(const std::string& text, PartOfSpeech pos, EffectTag effect, int effectValue);

    const std::string& getText() const;
    PartOfSpeech getPartOfSpeech() const;
    EffectTag getEffectTag() const;
    int getEffectValue() const;
};

#endif //UNTITLED1_WORD_H