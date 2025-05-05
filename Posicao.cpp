// Código atualizado com sequenciamento automático de pipetagem
#include "mbed.h"
#include "TextLCD.h"

#define velo 0.1
#define PASSO_MM 1.25f


#define velo 0.1 // tempo entre os passos
#define PASSO_MM 1.25f // 1 passo = 1.25mm

// LCD (RS, E, D4, D5, D6, D7)
TextLCD lcd(PA_5, PA_6, PA_7, PA_8, PA_9, PA_10);

// Controle de motor Z (mantido via BusOut)
BusOut MotorZ(D5, D4, D3, D2);

// Controle TB6560 - Eixo X
DigitalOut StepX(D8);
DigitalOut DirX(D9);
DigitalOut EnableX(D10); // opcional

// Controle TB6560 - Eixo Y
DigitalOut StepY(D11);
DigitalOut DirY(D12);
DigitalOut EnableY(D13); // opcional

// Botões de movimentação e input
DigitalIn BotaoXP(PA_0);
DigitalIn BotaoXN(PA_1);
DigitalIn BotaoYP(PA_2);
DigitalIn BotaoYN(PA_3);
DigitalIn BotaoZP(PA_4);
DigitalIn BotaoZN(PB_0);
DigitalIn BotaoInput(PB_12, PullUp); // botão para salvar posição

// Sensores de fim de curso
DigitalIn FdC_Z_Min(PC_0);
DigitalIn FdC_Z_Max(PC_1);
DigitalIn FdC_X_Max(A2);
DigitalIn FdC_X_Min(A3);
DigitalIn FdC_Y_Max(A5);
DigitalIn FdC_Y_Min(A4);

// Variáveis de posição (em passos)
int posicao_X = 0, posicao_Y = 0, posicao_Z = 0;
bool referenciado_X = false, referenciado_Y = false;


// Vetores e controle de posições salvas
const int tamanho_array = 100;
int desloc_X[tamanho_array], desloc_Y[tamanho_array];
int num_posicoes_salvas = 0;
bool posicao_de_coleta_salva = false;
int posicao_coletaX = 0, posicao_coletaY = 0;

void AcionamentoMotorX(int dir) {
    DirX = (dir == 0);
    StepX = 1; wait_ms(2);
    StepX = 0; wait_ms(2);
}

void AcionamentoMotorY(int dir) {
    DirY = (dir == 0);
    StepY = 1; wait_ms(2);
    StepY = 0; wait_ms(2);
}

void AcionamentoMotorZ(int estado) {
    int f[4] = {0x01, 0x02, 0x04, 0x08};
    switch (estado) {
        case 1: for (int i = 0; i < 4; i++) { MotorZ = f[i]; wait(velo); } posicao_Z++; break;
        case 2: for (int i = 3; i >= 0; i--) { MotorZ = f[i]; wait(velo); } posicao_Z--; break;
        default: MotorZ = 0; break;
    }
}

float passosParaMM(int passos) { return passos * PASSO_MM; }

void SalvarPosicaoCOLETA() {
    posicao_coletaX = posicao_X;
    posicao_coletaY = posicao_Y;
    posicao_de_coleta_salva = true;
    lcd.cls();
    lcd.printf("Coleta salva\nX=%.1fmm Y=%.1fmm", passosParaMM(posicao_X), passosParaMM(posicao_Y));
    wait_ms(2000);
    lcd.cls();
}

void SalvarPosicaoLiberacao() {
    if (num_posicoes_salvas < tamanho_array) {
        desloc_X[num_posicoes_salvas] = posicao_X - posicao_coletaX;
        desloc_Y[num_posicoes_salvas] = posicao_Y - posicao_coletaY;
        lcd.cls();
        lcd.printf("Lib %d salva\nDX=%d DY=%d", num_posicoes_salvas + 1, desloc_X[num_posicoes_salvas], desloc_Y[num_posicoes_salvas]);
        num_posicoes_salvas++;
        wait_ms(2000);
        lcd.cls();
    }
}

void MoverRelativoXY(int dx, int dy) {
    while (dx != 0) {
        if (dx > 0) { AcionamentoMotorX(1); posicao_X++; dx--; }
        else        { AcionamentoMotorX(0); posicao_X--; dx++; }
    }
    while (dy != 0) {
        if (dy > 0) { AcionamentoMotorY(1); posicao_Y++; dy--; }
        else        { AcionamentoMotorY(0); posicao_Y--; dy++; }
    }
}

void ExecutarSequencia() {
    for (int i = 0; i < num_posicoes_salvas; i++) {
        MoverRelativoXY(-desloc_X[i], -desloc_Y[i]); // volta para coleta
        wait_ms(500);
        MoverRelativoXY(desloc_X[i], desloc_Y[i]);  // vai para liberação
        wait_ms(500);
    }
    lcd.cls(); lcd.printf("Sequencia ok!");
    wait_ms(2000); lcd.cls();
}

int main() {
    lcd.cls(); lcd.printf("Defina coleta");

    while (!referenciado_X || !referenciado_Y) {
        if (BotaoXP == 0) { AcionamentoMotorX(1); posicao_X++; }
        else if (BotaoXN == 0) { AcionamentoMotorX(0); posicao_X--; }

        if (BotaoYP == 0) { AcionamentoMotorY(1); posicao_Y++; }
        else if (BotaoYN == 0) { AcionamentoMotorY(0); posicao_Y--; }

        if (BotaoInput == 0 && !posicao_de_coleta_salva) {
            SalvarPosicaoCOLETA();
            lcd.printf("Defina lib...");
            while (BotaoInput == 0); 
            wait_ms(300);
        } else if (BotaoInput == 0 && posicao_de_coleta_salva) {
            SalvarPosicaoLiberacao();
            while (BotaoInput == 0); 
            wait_ms(300);
        }
    }

    ExecutarSequencia();
    while (true);
}
