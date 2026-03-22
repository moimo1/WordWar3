#include "Player.h"
#include <iostream>

Player::Player(const std::string& name, int startHp) 
    : name(name), hp(startHp), shield(0) 
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
    }
}

void Player::reduceShield(int amount) {
    shield -= amount;
    if (shield < 0) shield = 0;
}

void Player::applySentenceEffects(const Sentence& sentence, Player& opponent) {
    std::cout << name << " played: \"" << sentence.toString() << "\"\n";

    // 1. Process Self-Effects (Heal, Defense)
    int heal = sentence.calculateTotalEffect(EffectType::Heal);
    if (heal > 0) {
        hp += heal;
        std::cout << name << " healed " << heal << " HP!\n";
    }

    int defense = sentence.calculateTotalEffect(EffectType::Defense);
    if (defense > 0) {
        addShield(defense);
        std::cout << name << " gained " << defense << " shield!\n";
    }

    // 2. Process Target-Effects (Weaken, Damage, Amplify, Critical)
    int weaken = sentence.calculateTotalEffect(EffectType::Weaken);
    if (weaken > 0) {
        opponent.reduceShield(weaken);
        std::cout << opponent.getName() << " was weakened, losing up to " << weaken << " shield!\n";
    }

    int damage = sentence.calculateTotalEffect(EffectType::Damage);
    int amplify = sentence.calculateTotalEffect(EffectType::Amplify);
    int critical = sentence.calculateTotalEffect(EffectType::Critical);

    if (damage > 0 || amplify > 0) {
        int totalDamage = damage + amplify;
        if (critical > 0) {
            totalDamage *= 2; // Critical is a simple 2x multiplier
            std::cout << "CRITICAL HIT! ";
        }
        std::cout << name << " attacks " << opponent.getName() << " for " << totalDamage << " damage!\n";
        opponent.takeDamage(totalDamage);
    }
}

void Player::displayStatus() const {
    std::cout << "[" << name << "] HP: " << hp << " | Shield: " << shield << "\n";
}