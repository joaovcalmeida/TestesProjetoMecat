// joystick.cpp
// Lê o joystick analógico em A0/A1 e comanda os motores X e Y

#include "mbed.h"
#include "joystick.h"  // Protótipo de joystick_control()

// Joystick nas portas analógicas
AnalogIn joystickX(A0);
AnalogIn joystickY(A1);

// Limites de passos para cada direção
const int MAX_PASSOS_X = 500;
const int MAX_PASSOS_Y = 500;

// Zona morta para evitar ruído no centro
const float DEAD_HIGH = 0.55f;
const float DEAD_LOW  = 0.45f;

// Sensores de fim de curso e posições (definidos em main.cpp)
extern DigitalIn FdC_X_Max;
extern DigitalIn FdC_X_Min;
extern DigitalIn FdC_Y_Max;
extern DigitalIn FdC_Y_Min;
extern int posicao_X;
extern int posicao_Y;

// Funções de passo (definidas em main.cpp)
extern void AcionamentoMotorX(int sentido);
extern void AcionamentoMotorY(int sentido);

// Função pública para integrar no laço principal
void joystick_control() {
    float x = joystickX.read();
    float y = joystickY.read();

    // Eixo X via joystick
    if (x > DEAD_HIGH && posicao_X < MAX_PASSOS_X && FdC_X_Max.read()) {
        AcionamentoMotorX(1);  // avança
        posicao_X++;
    } else if (x < DEAD_LOW && posicao_X > 0 && FdC_X_Min.read()) {
        AcionamentoMotorX(2);  // recua
        posicao_X--;
    }

    // Eixo Y via joystick
    if (y > DEAD_HIGH && posicao_Y < MAX_PASSOS_Y && FdC_Y_Max.read()) {
        AcionamentoMotorY(1);
        posicao_Y++;
    } else if (y < DEAD_LOW && posicao_Y > 0 && FdC_Y_Min.read()) {
        AcionamentoMotorY(2);
        posicao_Y--;
    }
}
