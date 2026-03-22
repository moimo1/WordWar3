#include "Effect.h"

Effect::Effect(EffectType type, int value) : type(type), value(value) {}

EffectType Effect::getType() const { return type; }
int Effect::getValue() const { return value; }

std::string Effect::getTypeName() const {
    switch(type) {
        case EffectType::Damage: return "Damage";
        case EffectType::Defense: return "Defense";
        case EffectType::Critical: return "Critical";
        case EffectType::Heal: return "Heal";
        case EffectType::Weaken: return "Weaken";
        case EffectType::Amplify: return "Amplify";
        default: return "None";
    }
}

nlohmann::json Effect::toJson() const {
    return {
        {"type", static_cast<int>(type)}, 
        {"value", value}
    };
}

Effect Effect::fromJson(const nlohmann::json& j) {
    return Effect(
        static_cast<EffectType>(j["type"].get<int>()), 
        j["value"].get<int>()
    );
}