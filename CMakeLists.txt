cmake_minimum_required(VERSION 4.1)
project(untitled1)

set(CMAKE_CXX_STANDARD 20)

add_executable(untitled1 main.cpp
        combat/Effect.cpp
        combat/Effect.h
        manager/GameManager.cpp
        manager/GameManager.h
        manager/UIManager.cpp
        manager/UIManager.h
        player/Player.cpp
        player/Player.h
        player/Sentence.cpp
        player/Sentence.h
        word/Word.cpp
        word/Word.h
        word/WordDictionary.cpp
        word/WordDictionary.h)
