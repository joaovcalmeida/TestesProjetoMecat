
#include "mbed.h"
#include "TextLCD.h"

#define velo 0.005f // tempo entre os passos
#define PASSO_MM 1.25f // 1 passo = 1.25mm

// LCD (RS, E, D4, D5, D6, D7)
TextLCD lcd(PA_5, PA_6, PA_7, PA_8, PA_9, PA_10);

DigitalOut LED(PA_5);

// Controle de motor Z (mantido via BusOut)
BusOut MotorZ(D5, D4, D3, D2);

// Controle TB6560 - Eixo X
DigitalOut StepX(D8);
DigitalOut DirX(D9);
DigitalOut EnableX(D10); // opcional

// Controle TB6560 - Eixo Y
DigitalOut StepY(D11);
DigitalOut DirY(D12);
DigitalOut EnableY(D6); // opcional

// Botões de input
DigitalIn BotaoZP(PA_4);
DigitalIn BotaoZN(PB_0);
DigitalIn BotaoInput(PB_12, PullUp); // botão para salvar posição

// Entradas do joystick (analógicas)
AnalogIn JoyX(A0);
AnalogIn JoyY(A1);

// Sensores de fim de curso
DigitalIn FdC_Z_Min(PC_0);
DigitalIn FdC_Z_Max(PC_1);
DigitalIn FdC_X_Max(A2);  //A2
DigitalIn FdC_X_Min(A3); //A3
DigitalIn FdC_Y_Max(A5); //A5
DigitalIn FdC_Y_Min(A4); //A4

// Variáveis de posição (em passos)
int posicao_X = 0, posicao_Y = 0, posicao_Z = 0;
bool referenciado_Z = false, referenciado_X = false, referenciado_Y = false;

// Vetores e controle de posições salvas
const int tamanho_array = 100;
int posicoes_X[tamanho_array], posicoes_Y[tamanho_array], posicoes_Z[tamanho_array];
int num_posicoes_salvas = 0;
bool posicao_de_coleta_salva = false;
int posicao_coletaX = 0, posicao_coletaY = 0, posicao_coletaZ = 0;

void AcionamentoMotorX(int sentido) {
    DirX = sentido;
    StepX = 1;
    wait_us(600);
    StepX = 0;
    wait_us(600);
}

void AcionamentoMotorY(int sentido) {
    DirY = sentido;
    StepY = 1;
    wait_us(1000);
    StepY = 0;
    wait_us(1000);
}

void AcionamentoMotorZ(int estado) {
    int f[4] = {0x01, 0x02, 0x04, 0x08};
    switch (estado) {
        case 1: for (int i = 0; i < 4; i++) { MotorZ = f[i]; wait(velo); } posicao_Z++; break;
        case 2: for (int i = 3; i >= 0; i--) { MotorZ = f[i]; wait(velo); } posicao_Z--; break;
        default: MotorZ = 0; break;
    }
}

void ReferenciarX() {
    LED = 1;
    if (FdC_X_Max == 1) { AcionamentoMotorX(0); }
    if (FdC_X_Max == 0) {
        posicao_X = 0;
        referenciado_X = true;
        lcd.cls(); lcd.printf("X referenciado\nX=0");
    }
}

void ReferenciarY() {
    LED = 0;
    if (FdC_Y_Min == 1) { AcionamentoMotorY(1); }
    if (FdC_Y_Min == 0) {
        posicao_Y = 0;
        referenciado_Y = true;
        lcd.cls(); lcd.printf("Y referenciado\nY=0");
    }
}

float passosParaMM(int passos) {
    return passos * PASSO_MM;
}

void SalvarPosicaoCOLETA() {
    posicao_coletaX = posicao_X;
    posicao_coletaY = posicao_Y;
    posicao_coletaZ = posicao_Z;
    posicao_de_coleta_salva = true;

    lcd.cls();
    lcd.locate(0, 0); lcd.printf("Coleta salva:");
    lcd.locate(0, 1); lcd.printf("X = %.1f mm", passosParaMM(posicao_coletaX));
    lcd.locate(0, 2); lcd.printf("Y = %.1f mm", passosParaMM(posicao_coletaY));
    lcd.locate(0, 3); lcd.printf("Z = %.1f mm", passosParaMM(posicao_coletaZ));
    wait_ms(3000);
    lcd.cls();
}

void SalvarPosicaoLiberacao() {
    if (num_posicoes_salvas < tamanho_array) {
        posicoes_X[num_posicoes_salvas] = posicao_X;
        posicoes_Y[num_posicoes_salvas] = posicao_Y;
        posicoes_Z[num_posicoes_salvas] = posicao_Z;

        lcd.cls();
        lcd.locate(0, 0); lcd.printf("Lib %d salva!", num_posicoes_salvas + 1);
        lcd.locate(0, 1); lcd.printf("X: %.1f mm", passosParaMM(posicao_X));
        lcd.locate(0, 2); lcd.printf("Y: %.1f mm", passosParaMM(posicao_Y));
        lcd.locate(0, 3); lcd.printf("Z: %.1f mm", passosParaMM(posicao_Z));

        num_posicoes_salvas++;

        wait_ms(1000);
        lcd.cls();
        if (num_posicoes_salvas < tamanho_array) {
            lcd.locate(0, 0); lcd.printf("Prox: pos %d", num_posicoes_salvas + 1);
        } else {
            lcd.locate(0, 0); lcd.printf("Pronto p/ pipetar");
        }
    } else {
        lcd.cls();
        lcd.locate(0, 0); lcd.printf("Limite atingido!");
        lcd.locate(0, 1); lcd.printf("Max: %d posicoes", tamanho_array);
        wait_ms(2000);
        lcd.cls();
    }
}

void MoverParaPosicaoFornecida(int alvo_X, int alvo_Y, int alvo_Z) {
    lcd.cls();
    lcd.locate(0, 0); lcd.printf("Indo para:");
    lcd.locate(0, 1); lcd.printf("X:%.1f Y:%.1f", passosParaMM(alvo_X), passosParaMM(alvo_Y));
    lcd.locate(0, 2); lcd.printf("Z:%.1f mm", passosParaMM(alvo_Z));

    while (posicao_X != alvo_X) {
        if (posicao_X < alvo_X) { AcionamentoMotorX(1); posicao_X++; }
        else { AcionamentoMotorX(0); posicao_X--; }
    }

    while (posicao_Y != alvo_Y) {
        if (posicao_Y < alvo_Y) { AcionamentoMotorY(1); posicao_Y++; }
        else { AcionamentoMotorY(0); posicao_Y--; }
    }

    while (posicao_Z != alvo_Z) {
        if (posicao_Z < alvo_Z) { AcionamentoMotorZ(1); posicao_Z++; }
        else { AcionamentoMotorZ(2); posicao_Z--; }
    }

    lcd.locate(0, 3); lcd.printf("Mov. finalizado!");
    wait_ms(1000);
    lcd.cls();
}

int main() {
    LED = 1;
    while (!referenciado_X) ReferenciarX();
    while (!referenciado_Y) ReferenciarY();

    while (true) {
        float leituraX = JoyX.read();
        float leituraY = JoyY.read();

        if (leituraY > 0.6f) { AcionamentoMotorY(1); posicao_Y++; }
        else if (leituraY < 0.4f) { AcionamentoMotorY(0); posicao_Y--; }

        if (leituraX > 0.6f) { AcionamentoMotorX(1); posicao_X++; }
        else if (leituraX < 0.4f) { AcionamentoMotorX(0); posicao_X--; }


        // if (BotaoZP == 0) AcionamentoMotorZ(1);
        // else if (BotaoZN == 0) AcionamentoMotorZ(2);
        // else MotorZ = 0;

        // if (BotaoInput == 0) {
        //     wait_ms(300);
        //     if (!posicao_de_coleta_salva) SalvarPosicaoCOLETA();
        //     else SalvarPosicaoLiberacao();
        //     while (BotaoInput == 0);
        //     wait_ms(100);
        }
    }
// }
