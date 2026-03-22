#include "Player.h"
#include <iostream>

Player::Player(const std::string& name, int startHp) 
    : name(name), hp(startHp), maxHp(startHp), shield(0), maxShield(startHp) 
{
}

const std::string& Player::getName() const { return name; }
int Player::getHp() const { return hp; }
int Player::getShield() const { return shield; }

void Player::takeDamage(int amount) {
    if (amount <= 0) return;
    if (shield > 0) {
        if (shield >= amount) {
            shield -= amount;
            amount = 0;
        } else {
            amount -= shield;
            shield = 0;
        }
    }
    hp -= amount;
    if (hp < 0) hp = 0;
}

void Player::addShield(int amount) {
    if (amount > 0) {
        shield += amount;
        if (shield > maxShield) shield = maxShield;
    }
}

void Player::reduceShield(int amount) {
    shield -= amount;
    if (shield < 0) shield = 0;
}

std::vector<std::string> Player::applySentenceEffects(const Sentence& sentence, Player& opponent) {
    std::vector<std::string> logs;
    logs.push_back(name + " played: \"" + sentence.toString() + "\"");

    int heal = sentence.calculateTotalEffect(EffectType::Heal);
    int healCount = sentence.getEffectWordCount(EffectType::Heal);
    if (heal > 0) {
        if (healCount >= 2) {
            int comboBonus = static_cast<int>(heal * (healCount - 1) * 0.5f);
            heal += comboBonus;
            logs.push_back("COMBO! " + std::to_string(healCount) + "x HEAL! (+" + std::to_string(comboBonus) + " Bonus)");
        }
        hp += heal;
        if (hp > maxHp) hp = maxHp;
        logs.push_back(name + " healed " + std::to_string(heal) + " HP! (Max: " + std::to_string(maxHp) + ")");
    }

    int defense = sentence.calculateTotalEffect(EffectType::Defense);
    int defCount = sentence.getEffectWordCount(EffectType::Defense);
    if (defense > 0) {
        if (defCount >= 2) {
            int comboBonus = static_cast<int>(defense * (defCount - 1) * 0.5f);
            defense += comboBonus;
            logs.push_back("COMBO! " + std::to_string(defCount) + "x DEFENSE! (+" + std::to_string(comboBonus) + " Bonus)");
        }
        addShield(defense);
        logs.push_back(name + " gained " + std::to_string(defense) + " shield!");
    }

    int weaken = sentence.calculateTotalEffect(EffectType::Weaken);
    int weakCount = sentence.getEffectWordCount(EffectType::Weaken);
    if (weaken > 0) {
        if (weakCount >= 2) {
            int comboBonus = static_cast<int>(weaken * (weakCount - 1) * 0.5f);
            weaken += comboBonus;
            logs.push_back("COMBO! " + std::to_string(weakCount) + "x WEAKEN! (+" + std::to_string(comboBonus) + " Bonus)");
        }
        opponent.reduceShield(weaken);
        logs.push_back(opponent.getName() + " was weakened, losing up to " + std::to_string(weaken) + " shield!");
    }

    int damage = sentence.calculateTotalEffect(EffectType::Damage);
    int amplify = sentence.calculateTotalEffect(EffectType::Amplify);
    int critical = sentence.calculateTotalEffect(EffectType::Critical);
    int dmgCount = sentence.getEffectWordCount(EffectType::Damage) + sentence.getEffectWordCount(EffectType::Amplify);

    if (damage > 0 || amplify > 0) {
        int totalDamage = damage + amplify;
        if (dmgCount >= 2) {
            int comboBonus = static_cast<int>(totalDamage * (dmgCount - 1) * 0.5f);
            totalDamage += comboBonus;
            logs.push_back("COMBO! " + std::to_string(dmgCount) + "x DAMAGE STREAM! (+" + std::to_string(comboBonus) + " Bonus)");
        }
        if (critical > 0) {
            totalDamage *= 2; 
            logs.push_back("CRITICAL HIT!");
        }
        logs.push_back(name + " attacks " + opponent.getName() + " for " + std::to_string(totalDamage) + " damage!");
        opponent.takeDamage(totalDamage);
    }
    
    return logs;
}

void Player::displayStatus() const {
    std::cout << "[" << name << "] HP: " << hp << " | Shield: " << shield << "\n";
}