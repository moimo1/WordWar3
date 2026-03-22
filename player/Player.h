#ifndef UNTITLED1_PLAYER_H
#define UNTITLED1_PLAYER_H

#include <string>
#include "Sentence.h"

class Player {
private:
    std::string name;
    int hp;
    int shield;

public:
    Player(const std::string& name, int startHp);

    const std::string& getName() const;
    int getHp() const;
    int getShield() const;

    void takeDamage(int amount);
    void addShield(int amount);
    void reduceShield(int amount);

    void applySentenceEffects(const Sentence& sentence, Player& opponent);
    void displayStatus() const;
};

#endif //UNTITLED1_PLAYER_H