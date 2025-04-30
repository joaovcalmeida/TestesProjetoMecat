#include "mbed.h"
#include "TextLCD.h"

#define velo 0.1 // tempo entre os passos
#define PASSO_MM 1.25f // 1 passo = 1.25mm

// Serial PC(USBTX,USBRX);

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
int estado_Z = 0, estado_X = 0, estado_Y = 0;
bool referenciado_Z = false, referenciado_X = false, referenciado_Y = false;

// Vetores e controle de posições salvas
const int tamanho_array = 100;
int posicoes_X[tamanho_array], posicoes_Y[tamanho_array], posicoes_Z[tamanho_array];
int num_posicoes_salvas = 0;
bool posicao_de_coleta_salva = false;
int posicao_coletaX = 0, posicao_coletaY = 0, posicao_coletaZ = 0;

// Prototipagem de funções
void AcionamentoMotorX(int sentido);
void AcionamentoMotorY(int sentido);
void AcionamentoMotorZ(int estado);


// === NOVAS FUNÇÕES ADAPTADAS PARA TB6560 ===

void AcionamentoMotorX(int sentido) {
    DirX = sentido;  // 0 = horário, 1 = anti-horário
    StepX = 1;
    wait_ms(1);
    StepX = 0;
    wait_ms(1);
}

void AcionamentoMotorY(int sentido) {
    DirY = sentido;
    StepY = 1;
    wait_ms(1);
    StepY = 0;
    wait_ms(1);
}

// Motor Z mantém controle via BusOut
void AcionamentoMotorZ(int estado) {
    int f[4] = {0x01, 0x02, 0x04, 0x08};
    switch (estado) {
        case 1: for (int i = 0; i < 4; i++) { MotorZ = f[i]; wait(velo); } posicao_Z++; break;
        case 2: for (int i = 3; i >= 0; i--) { MotorZ = f[i]; wait(velo); } posicao_Z--; break;
        default: MotorZ = 0; break;
    }
}

// Referenciamento
void ReferenciarZ() {
    if (FdC_Z_Max == 1) {AcionamentoMotorX(1);
    }
    if (FdC_Z_Max == 0){
        posicao_Z = 0;
        referenciado_Z = true;
        lcd.cls(); lcd.printf("Z referenciado\nX=0");
        }
}

void ReferenciarX() { 
    LED = 1;
    if (FdC_X_Max == 1) {AcionamentoMotorX(0);
    }
    if (FdC_X_Max == 0){
        posicao_X = 0;
        referenciado_X = true;
        lcd.cls(); lcd.printf("X referenciado\nX=0");
        }
}



void ReferenciarY() {
    LED = 0;

    if (FdC_Y_Min == 1) {AcionamentoMotorY(1);
    }
    if (FdC_Y_Min == 0){
        posicao_Y = 0;
        referenciado_Y = true;
        lcd.cls(); lcd.printf("Y referenciado\nX=0");
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
        else { AcionamentoMotorX(2); posicao_X--; }
    }

    while (posicao_Y != alvo_Y) {
        if (posicao_Y < alvo_Y) { AcionamentoMotorY(1); posicao_Y++; }
        else { AcionamentoMotorY(2); posicao_Y--; }
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
    //while (!referenciado_Z) ReferenciarZ();
    while (!referenciado_X) ReferenciarX();
    while (!referenciado_Y) ReferenciarY();

    while (true) {
        if (BotaoXP == 0) { AcionamentoMotorX(1); posicao_X++; }
        else if (BotaoXN == 0) { AcionamentoMotorX(2); posicao_X--; }

        if (BotaoYP == 0) { AcionamentoMotorY(1); posicao_Y++; }
        else if (BotaoYN == 0) { AcionamentoMotorY(2); posicao_Y--; }

        if (BotaoZP == 0) AcionamentoMotorZ(1);
        else if (BotaoZN == 0) AcionamentoMotorZ(2);
        else MotorZ = 0;

        if (BotaoInput == 0) {
            wait_ms(300);
            if (!posicao_de_coleta_salva) SalvarPosicaoCOLETA();
            else SalvarPosicaoLiberacao();
            while (BotaoInput == 0);
            wait_ms(100);
        }
    }
}