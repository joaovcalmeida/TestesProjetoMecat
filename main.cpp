#include "mbed.h"
#include "TextLCD.h"

#define velo 0.002f
#define PASSO_MM 0.025f
#define ALTURA_Z_SEGURA 80

// === Hardware definitions ===
I2C i2c_lcd(I2C_SDA, I2C_SCL);
TextLCD_I2C lcd(&i2c_lcd, 0x4e, TextLCD::LCD20x4);

BusOut    MotorZ(D5, D4, D3, D2);
DigitalOut StepX(D8), DirX(D9), EnableX(D10);
DigitalOut StepY(D11), DirY(D12), EnableY(D6);

DigitalIn BotaoZP(PB_13, PullUp), BotaoZN(PB_14, PullUp), BotaoInput(PC_13, PullUp);
AnalogIn  JoyX(A0), JoyY(A1);

DigitalIn FdC_Z_Min(PC_2), FdC_Z_Max(PC_3);
DigitalIn FdC_X_Max(A2), FdC_X_Min(A3);
DigitalIn FdC_Y_Max(A5), FdC_Y_Min(A4);

InterruptIn encoderA(PA_12);
DigitalIn    encoderB(PA_11);

// === State variables ===
int  posicao_X = 0, posicao_Y = 0, posicao_Z = 0;
bool referenciado_X = false, referenciado_Y = false, referenciado_Z = false;

const int tamanho_array = 100;
int posicoes_X[tamanho_array], posicoes_Y[tamanho_array], posicoes_Z[tamanho_array];
int volumes[tamanho_array];
int num_posicoes_salvas = 0;
bool posicao_de_coleta_salva = false;
int  posicao_coletaX = 0, posicao_coletaY = 0, posicao_coletaZ = 0;

int num_pontos_lib = 0;
volatile int encoder_val = 0;

// === Routines ===

// Encoder rotary → ml selector
void encoderSubir() {
    if (encoderB.read() == 0 && encoder_val < 10) encoder_val++;
    else if (encoder_val > 0)                 encoder_val--;
}

// Stepper X/Y/Z low‐level
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
    static const int seq[4] = {0x01,0x02,0x04,0x08};
    if (estado==1) {
        for (int i=0; i<4; i++) { MotorZ = seq[i]; wait(velo); }
        posicao_Z++;
    }
    else if (estado==2) {
        for (int i=3; i>=0; i--) { MotorZ = seq[i]; wait(velo); }
        posicao_Z--;
    }
    MotorZ = 0;
}

// Reference Z axis at top limit
void ReferenciarZ() {
    // Sobe até o fim de curso superior
    while (FdC_Z_Max.read() == 0) {
        AcionamentoMotorZ(1);
    }
    posicao_Z = 0;
    referenciado_Z = true;
    // opcional: aguardar estabilizar
    wait_ms(100);
}

// Move to (x,y) safely: always reference Z, then XY, then descend Z to target
void MoverParaComSeguranca(int x, int y, int z_destino) {
    // 1) sobe ao topo
    ReferenciarZ();

    // 2) move X
    while (posicao_X != x) {
        if (posicao_X < x) { AcionamentoMotorX(1); posicao_X++; }
        else               { AcionamentoMotorX(0); posicao_X--; }
    }
    // 3) move Y
    while (posicao_Y != y) {
        if (posicao_Y < y) { AcionamentoMotorY(1); posicao_Y++; }
        else               { AcionamentoMotorY(0); posicao_Y--; }
    }
    // 4) desce até z_destino (z_destino é ≤ 0 se abaixo do topo)
    while (posicao_Z > z_destino) {
        AcionamentoMotorZ(2);
    }
}

// Reference X/Y axes
void ReferenciarX() {
    while (FdC_X_Max.read() == 1) { AcionamentoMotorX(0); }
    posicao_X = 0;
    referenciado_X = true;
    lcd.cls(); lcd.printf("X referenciado\nX=0");
    wait_ms(2000);
}

void ReferenciarY() {
    while (FdC_Y_Min.read() == 1) { AcionamentoMotorY(1); }
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
    lcd.locate(0,0); lcd.printf("Coleta salva:");
    lcd.locate(0,1); lcd.printf("X=%.1f mm", passosParaMM(posicao_coletaX));
    lcd.locate(0,2); lcd.printf("Y=%.1f mm", passosParaMM(posicao_coletaY));
    lcd.locate(0,3); lcd.printf("Z=%.1f mm", passosParaMM(posicao_coletaZ));
    wait_ms(2000);

    // após salvar, retira Z para topo
    ReferenciarZ();
}

int main() {
    // Setup LCD e encoder
    lcd.setCursor(TextLCD::CurOff_BlkOn);
    lcd.setBacklight(TextLCD::LightOn);
    encoderA.rise(&encoderSubir);

    // 1) Referenciar todas as bases
    lcd.cls();
    lcd.locate(0,0); lcd.printf("Deseja referenciar?");
    lcd.locate(0,1); lcd.printf("Pressione INPUT");
    while (BotaoInput.read()==1);
    wait_ms(300);
    while (BotaoInput.read()==0);
    ReferenciarZ();
    ReferenciarX();
    ReferenciarY();

    // 2) Selecionar posição de coleta
    lcd.cls();
    lcd.locate(0,0); lcd.printf("Selecione coleta");
    lcd.locate(0,1); lcd.printf("Pressione INPUT");
    while (!posicao_de_coleta_salva) {
        float lx = JoyX.read(), ly = JoyY.read();
        if (ly>0.6f) { AcionamentoMotorY(1); posicao_Y++; }
        else if (ly<0.4f) { AcionamentoMotorY(0); posicao_Y--; }
        if (lx>0.6f) { AcionamentoMotorX(1); posicao_X++; }
        else if (lx<0.4f) { AcionamentoMotorX(0); posicao_X--; }

        // Z manual
        if (BotaoZP.read() == 0 && FdC_Z_Max.read() == 0) { AcionamentoMotorZ(1); }
        else if (BotaoZN.read() == 0 && FdC_Z_Min.read() == 0) { AcionamentoMotorZ(2); }

        else AcionamentoMotorZ(0);

        if (BotaoInput.read()==0) {
            wait_ms(300);
            SalvarPosicaoCOLETA();
            while (BotaoInput.read()==0);
            wait_ms(100);
        }
    }

    // 3) Quantos pontos de depósito
    lcd.cls();
    lcd.locate(0,0); lcd.printf("Quantos pontos");
    lcd.locate(0,1); lcd.printf("de deposito?");
    while (true) {
        lcd.locate(0,2); lcd.printf("Selecionado:%2d", encoder_val);
        wait_ms(300);
        if (BotaoInput.read()==0) {
            num_pontos_lib = encoder_val;
            while (BotaoInput.read()==0);
            break;
        }
    }

    // 4) Salvar cada ponto de depósito + volume
    for (int i = 0; i < num_pontos_lib; i++) {
        // posição
        lcd.cls();
        lcd.locate(0,0); lcd.printf("Ponto %d: selecione", i+1);
        while (true) {
            float lx = JoyX.read(), ly = JoyY.read();

            // Controle de X e Y via joystick
            if (ly > 0.6f)        { AcionamentoMotorY(1); posicao_Y++; }
            else if (ly < 0.4f)   { AcionamentoMotorY(0); posicao_Y--; }

            if (lx > 0.6f)        { AcionamentoMotorX(1); posicao_X++; }
            else if (lx < 0.4f)   { AcionamentoMotorX(0); posicao_X--; }

            // Controle manual de Z via botões ZP e ZN
            if (BotaoZP.read() == 1 && FdC_Z_Max.read() == 0) { AcionamentoMotorZ(1); }
            else if (BotaoZN.read() == 1 && FdC_Z_Min.read() == 0) { AcionamentoMotorZ(2); }
            else AcionamentoMotorZ(0); // para o motor Z se nenhum botão for pressionado

            // Confirmação de posição
            if (BotaoInput.read() == 0) {
                posicoes_X[i] = posicao_X;
                posicoes_Y[i] = posicao_Y;
                posicoes_Z[i] = posicao_Z;

                lcd.cls();
                lcd.printf("X=%.1f Y=%.1f", passosParaMM(posicao_X), passosParaMM(posicao_Y));
                lcd.locate(0,1); lcd.printf("Z=%.1f mm", passosParaMM(posicao_Z));
                wait_ms(2000);
                while (BotaoInput.read() == 0);
                break;
            }
        }

        // volume
        lcd.cls();
        lcd.locate(0,0); lcd.printf("Qnt ml ponto %d?", i+1);
        encoder_val = 0;
        while (true) {
            lcd.locate(0,1); lcd.printf("Selecionado:%2d ml", encoder_val);
            wait_ms(300);
            if (BotaoInput.read()==0) {
                volumes[i] = encoder_val;
                while (BotaoInput.read()==0);
                wait_ms(300);
                break;
            }
        }
    }

    // 5) Executar pipetagem
    lcd.cls(); lcd.printf("Iniciando fluxo...");
    wait_ms(2000);
    for (int i = 0; i < num_pontos_lib; i++) {
        int viagens = volumes[i];
        if (viagens <= 0 || viagens > 50) viagens = 1;  // segurança
        for (int v = 0; v < viagens; v++) {
            // volta à coleta
            lcd.cls(); lcd.printf("Ponto Coleta");
            MoverParaComSeguranca(posicao_coletaX, posicao_coletaY, posicao_coletaZ);
            wait_ms(500);
            // vai ao ponto de depósito
            lcd.cls(); lcd.printf("Ponto %d", i+1);
            MoverParaComSeguranca(posicoes_X[i], posicoes_Y[i], posicoes_Z[i]);
            wait_ms(500);
        }
    }

    // Final
    lcd.cls(); lcd.printf("Pipetagem OK!");
    while (true);
}