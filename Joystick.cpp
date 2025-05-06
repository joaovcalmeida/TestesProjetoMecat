// joystick.cpp
#include "mbed.h"
#include "Joystick.h"

// Joystick nas portas analógicas
static AnalogIn joystickX(A0);
static AnalogIn joystickY(A1);

// Limites de passos para cada direção
static const int MAX_PASSOS_X = 500;
static const int MAX_PASSOS_Y = 500;

// Zona morta para evitar ruído no centro
static const float DEAD_HIGH = 0.55f;
static const float DEAD_LOW  = 0.45f;

// Sensores de fim de curso e variáveis de posição (definidos em main.cpp)
extern DigitalIn FdC_X_Max;
extern DigitalIn FdC_X_Min;
extern DigitalIn FdC_Y_Max;
extern DigitalIn FdC_Y_Min;
extern int posicao_X;
extern int posicao_Y;

// Funções de passo (definidas em main.cpp)
extern void AcionamentoMotorX(int sentido);
extern void AcionamentoMotorY(int sentido);

/**
 * joystick_control
 * - Lê valores (0.0 a 1.0) do joystickX/Y
 * - Se fora da dead-zone, move X ou Y se dentro dos limites e fim de curso livre
 * - posicao_X e posicao_Y são atualizadas
 */
void joystick_control() {
    float x = joystickX.read();
    float y = joystickY.read();

    // Controle do eixo X
    if (x > DEAD_HIGH && posicao_X < MAX_PASSOS_X && FdC_X_Max.read()) {
        AcionamentoMotorX(1);  // avanço
        posicao_X++;
    }
    else if (x < DEAD_LOW && posicao_X > 0 && FdC_X_Min.read()) {
        AcionamentoMotorX(2);  // recuo
        posicao_X--;
    }

    // Controle do eixo Y
    if (y > DEAD_HIGH && posicao_Y < MAX_PASSOS_Y && FdC_Y_Max.read()) {
        AcionamentoMotorY(1);
        posicao_Y++;
    }
    else if (y < DEAD_LOW && posicao_Y > 0 && FdC_Y_Min.read()) {
        AcionamentoMotorY(2);
        posicao_Y--;
    }
}
