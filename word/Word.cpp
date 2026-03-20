//
// Created by ASUS TUF F15 on 3/18/2026.
//
#include "Word.h"

Word::Word(const std::string& text, PartOfSpeech pos, EffectTag effect, int effectValue)
    : text(text), pos(pos), effect(effect), effectValue(effectValue) 
{

}

const std::string& Word::getText() const {
    return text;
}

PartOfSpeech Word::getPartOfSpeech() const {
    return pos;
}

EffectTag Word::getEffectTag() const {
    return effect;
}

int Word::getEffectValue() const {
    return effectValue;
}