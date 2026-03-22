#ifndef COMBAT_EFFECT_H
#define COMBAT_EFFECT_H

#include <string>
#include <nlohmann/json.hpp>

enum class EffectType {
    None, Damage, Defense, Critical, Heal, Weaken, Amplify
};

class Effect {
private:
    EffectType type;
    int value;

public:
    Effect(EffectType type = EffectType::None, int value = 0);

    EffectType getType() const;
    int getValue() const;
    std::string getTypeName() const;

    // Networking serialization capabilities
    nlohmann::json toJson() const;
    static Effect fromJson(const nlohmann::json& j);
};

#endif // COMBAT_EFFECT_H