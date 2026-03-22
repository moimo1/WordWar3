#ifndef COMBAT_EFFECT_H
#define COMBAT_EFFECT_H

#include <string>

// The different types of effects a word can have in combat
enum class EffectType {
    None,
    Damage,    // Direct damage
    Defense,   // Adds shield
    Critical,  // If present, doubles damage
    Heal,      // Restores HP
    Weaken,    // Removes opponent's shield
    Amplify    // Adds flat bonus to damage before critical
};

// The Effect class groups the type and the numerical value together
class Effect {
private:
    EffectType type;
    int value;

public:
    // Default constructor is None with 0 value
    Effect(EffectType type = EffectType::None, int value = 0);

    EffectType getType() const;
    int getValue() const;
    std::string getTypeName() const;
};

#endif // COMBAT_EFFECT_H