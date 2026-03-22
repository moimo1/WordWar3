#ifndef UNTITLED1_PLAYER_H
#define UNTITLED1_PLAYER_H

#include <string>
#include <vector>
#include "Sentence.h"

class Player {
private:
    std::string name;
    int hp;
    int maxHp;
    int shield;
    int maxShield;

public:
    Player(const std::string& name, int startHp);

    const std::string& getName() const;
    int getHp() const;
    int getShield() const;

    void takeDamage(int amount);
    void addShield(int amount);
    void reduceShield(int amount);

    std::vector<std::string> applySentenceEffects(const Sentence& sentence, Player& opponent);
    void displayStatus() const;
};

#endif //UNTITLED1_PLAYER_H