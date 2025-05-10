#include "mbed.h"
#include "TextLCD.h"

#define velo 0.005f
#define PASSO_MM 0.025f

I2C i2c_lcd(I2C_SDA, I2C_SCL);
TextLCD_I2C lcd(&i2c_lcd, 0x4e, TextLCD::LCD20x4);

BusOut MotorZ(D5, D4, D3, D2);
DigitalOut StepX(D8), DirX(D9), EnableX(D10);
DigitalOut StepY(D11), DirY(D12), EnableY(D6);

DigitalIn BotaoZP(PA_4);
DigitalIn BotaoZN(PB_0);
DigitalIn BotaoInput(PC_13, PullUp);

AnalogIn JoyX(A0);
AnalogIn JoyY(A1);

DigitalIn FdC_Z_Min(PC_0), FdC_Z_Max(PC_1);
DigitalIn FdC_X_Max(A2), FdC_X_Min(A3);
DigitalIn FdC_Y_Max(A5), FdC_Y_Min(A4);

InterruptIn encoderA(PA_12);
DigitalIn encoderB(PA_11);

int posicao_X = 0, posicao_Y = 0, posicao_Z = 0;
bool referenciado_X = false, referenciado_Y = false, referenciado_Z = false;

const int tamanho_array = 100;
int posicoes_X[tamanho_array], posicoes_Y[tamanho_array], posicoes_Z[tamanho_array];
int num_posicoes_salvas = 0;
bool posicao_de_coleta_salva = false;
int posicao_coletaX = 0, posicao_coletaY = 0, posicao_coletaZ = 0;

int num_pontos_lib = 0;
volatile int encoder_val = 0;

void encoderSubir() {
    if (encoderB == 0 && encoder_val < 10) encoder_val++;
    else if (encoder_val > 0) encoder_val--;
}

void AcionamentoMotorX(int sentido) {
    DirX = sentido;
    StepX = 1; wait_us(600);
    StepX = 0; wait_us(600);
}

void AcionamentoMotorY(int sentido) {
    DirY = sentido;
    StepY = 1; wait_us(1000);
    StepY = 0; wait_us(1000);
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
    while (FdC_X_Max == 1) { AcionamentoMotorX(0); }
    posicao_X = 0;
    referenciado_X = true;
    lcd.cls(); lcd.printf("X referenciado\nX=0");
    wait_ms(2000);
}

void ReferenciarY() {
    while (FdC_Y_Min == 1) { AcionamentoMotorY(1); }
    posicao_Y = 0;
    referenciado_Y = true;
    lcd.cls(); lcd.printf("Y referenciado\nY=0");
    wait_ms(2000);
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
    wait_ms(2000);
}

int main() {
    lcd.setCursor(TextLCD::CurOff_BlkOn);
    lcd.setBacklight(TextLCD::LightOn);

    encoderA.rise(&encoderSubir);

    while (!referenciado_X) ReferenciarX();
    while (!referenciado_Y) ReferenciarY();

    lcd.cls();
    lcd.locate(0, 0); lcd.printf("Selecione coleta");
    lcd.locate(0, 1); lcd.printf("Pressione INPUT");

    while (!posicao_de_coleta_salva) {
        float leituraX = JoyX.read();
        float leituraY = JoyY.read();

        if (leituraY > 0.6f) { AcionamentoMotorY(1); posicao_Y++; }
        else if (leituraY < 0.4f) { AcionamentoMotorY(0); posicao_Y--; }

        if (leituraX > 0.6f) { AcionamentoMotorX(1); posicao_X++; }
        else if (leituraX < 0.4f) { AcionamentoMotorX(0); posicao_X--; }

        if (BotaoInput == 0) {
            wait_ms(300);
            SalvarPosicaoCOLETA();
            while (BotaoInput == 0);
            wait_ms(100);
        }
    }

    // --- Encoder para número de pontos ---
    lcd.cls();
    lcd.locate(0, 0); lcd.printf("Quantos pontos?");

    while (true) {
        lcd.locate(0, 1); lcd.printf("Selecionado: %d ", encoder_val);
        wait_ms(500);
        if (BotaoInput == 0) {
            num_pontos_lib = encoder_val;
            while (BotaoInput == 0);
            break;
        }
    }

    lcd.cls();
    lcd.printf("Selecione ponto 1");

    while (true) {
        float leituraX = JoyX.read();
        float leituraY = JoyY.read();

        if (leituraY > 0.6f) { AcionamentoMotorY(1); posicao_Y++; }
        else if (leituraY < 0.4f) { AcionamentoMotorY(0); posicao_Y--; }

        if (leituraX > 0.6f) { AcionamentoMotorX(1); posicao_X++; }
        else if (leituraX < 0.4f) { AcionamentoMotorX(0); posicao_X--; }

        if (BotaoZP == 0) AcionamentoMotorZ(1);
        else if (BotaoZN == 0) AcionamentoMotorZ(2);
        else MotorZ = 0;

        if (BotaoInput == 0) {
            wait_ms(300);
            // SalvarPosicaoLiberacao();
            while (BotaoInput == 0);
            wait_ms(100);
        }
    }
}
