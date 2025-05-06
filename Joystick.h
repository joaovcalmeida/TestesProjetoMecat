// joystick.h
#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "mbed.h"

/**
 * joystick_control()
 * Lê o joystick analógico nos pinos A0/A1 e comanda os motores X e Y.
 * Deve ser chamado dentro do loop principal de main.cpp.
 */
void joystick_control();

#endif // JOYSTICK_H
