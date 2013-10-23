#ifndef UTILITY_H
#define UTILITY_H

#include <QColor>
#include <cstdint>
void generateRandomSeed();
int randomInteger(int low, int high);
QColor getColor(uint8_t value);
#endif // UTILITY_H
