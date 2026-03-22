#include "Word.h"

Word::Word(const std::string& text, PartOfSpeech pos, Effect effect)
    : text(text), pos(pos), effect(effect) {}

const std::string& Word::getText() const { return text; }
PartOfSpeech Word::getPartOfSpeech() const { return pos; }
const Effect& Word::getEffect() const { return effect; }

nlohmann::json Word::toJson() const {
    return {
        {"text", text},
        {"pos", static_cast<int>(pos)},
        {"effect", effect.toJson()}
    };
}

Word Word::fromJson(const nlohmann::json& j) {
    return Word(
        j["text"].get<std::string>(),
        static_cast<PartOfSpeech>(j["pos"].get<int>()),
        Effect::fromJson(j["effect"])
    );
}