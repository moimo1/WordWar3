#include "Word.h"

Word::Word(const std::string& text, PartOfSpeech pos, Effect effect)
    : text(text), pos(pos), effect(effect) 
{
}

const std::string& Word::getText() const {
    return text;
}

PartOfSpeech Word::getPartOfSpeech() const {
    return pos;
}

const Effect& Word::getEffect() const {
    return effect;
}