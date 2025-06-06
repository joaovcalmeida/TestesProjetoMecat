#include "mbed.h"
#include "TextLCD.h"

#define velo 0.003f
#define PASSO_MM 0.025f


//Definição das variáveis e suas portas lógicas
I2C i2c_lcd(I2C_SDA, I2C_SCL);
TextLCD_I2C lcd(&i2c_lcd, 0x4e, TextLCD::LCD20x4);

DigitalOut Pipeta(PA_5);

BusOut MotorZ(PB_4, PB_5, PB_3, PA_10);
DigitalOut StepX(PA_9), DirX(PC_7), EnableX(PB_6);
DigitalOut StepY(PA_7), DirY(PA_6), EnableY(PB_10);

DigitalIn BotaoZP(PB_13, PullUp);
DigitalIn BotaoZN(PB_14, PullUp);
DigitalIn BotaoEncoder(PB_11, PullUp);

DigitalIn BotaoEmergencia(PC_4, PullUp);
DigitalOut Buzzer(PB_1);
AnalogIn JoyX(PA_0);
AnalogIn JoyY(PA_1);

DigitalIn FdC_Z_Min(PC_2), FdC_Z_Max(PC_3);
DigitalIn FdC_X_Max(PC_1), FdC_X_Min(PC_0);
DigitalIn FdC_Y_Max(PA_4), FdC_Y_Min(PB_0);

InterruptIn encoderA(PA_12);
DigitalIn encoderB(PA_11);

int posicao_X = 0, posicao_Y = 0, posicao_Z = 0;
bool referenciado_X = false, referenciado_Y = false, referenciado_Z = false;

const int tamanho_array = 100;
int posicoes_X[tamanho_array], posicoes_Y[tamanho_array], posicoes_Z[tamanho_array];
int volumes[tamanho_array];
int num_posicoes_salvas = 0;
bool posicao_de_coleta_salva = false;
int posicao_coletaX = 0, posicao_coletaY = 0, posicao_coletaZ = 0;

int num_pontos_lib = 0;
volatile int encoder_val = 0;

bool emergencia_ativa = false;
bool solicitar_referenciamento = false;


//Função para acionar a pipeta para a coleta ou depósito
void AcionarPipeta_Toggle() {
    Pipeta = 0;
    wait_ms(1200);  // Tempo suficiente para a pipeta alternar o estado
    Pipeta = 1;
    wait_ms(600);  // Delay para garantir estabilidade
}

//Função que permite a mudanção de valores do armazenados pelo enconder, de acordo com sua rotação
void encoderSubir() {
    if (encoderB == 0 && encoder_val < 10) encoder_val++;
    else if (encoder_val > 0) encoder_val--;
}

//Aciona Motor X
void AcionamentoMotorX(int sentido) {

    if (FdC_X_Max == 0 && sentido == 0) return;
    if (FdC_X_Min == 0 && sentido == 1) return;

    DirX = sentido;
    StepX = 1; wait_us(600);
    StepX = 0; wait_us(600);
}

//Aciona Motor Y
void AcionamentoMotorY(int sentido) {
    DirY = sentido;

    if (FdC_Y_Max == 0 && sentido == 0) return;
    if (FdC_Y_Min == 0 && sentido == 1) return;

    StepY = 1; wait_us(1000);
    StepY = 0; wait_us(1000);
}

//Aciona Motor Z, utilizando lógica diferente, tendo em vista que não utilizou-se nesse caso de um driver comercial
void AcionamentoMotorZ(int estado) {

    int f[4] = {0x01, 0x02, 0x04, 0x08};
    switch (estado) {
        case 1: for (int i = 0; i < 4; i++) { MotorZ = f[i]; wait(velo); } posicao_Z++; break;
        case 2: for (int i = 3; i >= 0; i--) { MotorZ = f[i]; wait(velo); } posicao_Z--; break;
    }
    MotorZ = 0;
}

//Função responsável por avaliar se o botão de emergência está acionado para iniciar a rotina de emergência
void VerificarEmergencia() {
    if (BotaoEmergencia == 0) {
        EnableX = 1;
        EnableY = 1;
        MotorZ = 0;
        emergencia_ativa = true;
        lcd.cls(); lcd.printf("EMERGENCIA!\nSolte o botao");

        // Espera soltar o botão de emergência
        while (BotaoEmergencia == 0) {
            Buzzer = 1;
            wait_ms(200);
            Buzzer = 0;
            wait_ms(200);
        }

        // Após soltar, solicita reset
        lcd.cls(); lcd.printf("Aperte INPUT\np/ resetar");
        while (BotaoEncoder == 1);
        wait_ms(300);
        while (BotaoEncoder == 0);
    

        // Marca como ativo para novo referenciamento
        emergencia_ativa = false;
        EnableX = 0;
        EnableY = 0;
        MotorZ = 0;
        solicitar_referenciamento = false;

        // Reseta flags e posições
        referenciado_X = false;
        referenciado_Y = false;
        referenciado_Z = false;
        posicao_X = 0;
        posicao_Y = 0;
        posicao_Z = 0;
        posicao_de_coleta_salva = false;

        lcd.cls();
        lcd.printf("Reset OK!");
        wait_ms(1000);

        // Início do referenciamento incorporado (sem chamada de função)
        lcd.cls();
        lcd.printf("Referenciando...");

    NVIC_SystemReset();
    //Reinicialização do código

    }
}

//Referenciamento do Eixo X
void ReferenciarX() {
    int etapa = 1;
    while (!referenciado_X) {
        VerificarEmergencia();
        switch (etapa) {
            case 1:
                if (FdC_X_Max == 1) AcionamentoMotorX(0);
                else etapa = 2;
                break;

            case 2:
                {
                    Timer t;
                    t.start();
                    while (t.read_ms() < 300) AcionamentoMotorX(1);
                    t.stop();
                    etapa = 3;
                }
                break;

            case 3:
                posicao_X = 0;
                referenciado_X = true;
                EnableX = 0;
                lcd.cls(); lcd.printf("X referenciado\nX=0");
                wait_ms(2000);
                break;
        }
    }
}

//Referenciamento do Eixo Y
void ReferenciarY() {
    int etapa = 1;
    while (!referenciado_Y) {
        VerificarEmergencia();
        switch (etapa) {
            case 1:
                if (FdC_Y_Min == 1) AcionamentoMotorY(1);
                else etapa = 2;
                break;

            case 2:
                {
                    Timer t;
                    t.start();
                    while (t.read_ms() < 200) AcionamentoMotorY(0);
                    t.stop();
                    etapa = 3;
                }
                break;

            case 3:
                posicao_Y = 0;
                referenciado_Y = true;
                EnableY = 0;
                lcd.cls(); lcd.printf("Y referenciado\nY=0");
                wait_ms(2000);
                break;
        }
    }
}

//Referenciamento do Eixo Z
void ReferenciarZ() {
    int etapa = 1;
    while (!referenciado_Z) {
        VerificarEmergencia();
        switch (etapa) {
            case 1:
                if (FdC_Z_Max == 1) AcionamentoMotorZ(1);
                else etapa = 2;
                break;

            case 2:
                {
                    Timer t;
                    t.start();
                    while (t.read_ms() < 300) AcionamentoMotorZ(2);
                    t.stop();
                    etapa = 3;
                }
                break;

            case 3:
                posicao_Z = 0;
                referenciado_Z = true;
                MotorZ = 0;
                lcd.cls(); lcd.printf("Z referenciado\nZ=0");
                wait_ms(2000);
                break;
        }
    }
}


//Antes de iniciar o ciclo automático, deve-se iniciar a subida do eixo Z para evitar choque com os componentes que podem estar na mesa
void ElevarZ_AteTopo() {
    while (FdC_Z_Max == 1) {
        VerificarEmergencia();
        AcionamentoMotorZ(1);
    }
    posicao_Z = 0;
}

//Calcula o número de MM andados de acordo o número de passos realizados
float passosParaMM(int passos) { 
    return passos * PASSO_MM;
}

//Salava a posição de coleta
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

//Função responsável por movimentação até os pontos de coleta e depósito
void MoverPara(float x, float y, float z) {
    // Primeiro eleva Z para evitar colisão com frascos
    ElevarZ_AteTopo();
    VerificarEmergencia();

    while (posicao_X != x) {
        VerificarEmergencia();
        if (posicao_X < x) { AcionamentoMotorX(1); posicao_X++; }
        else { AcionamentoMotorX(0); posicao_X--; }
    }

    while (posicao_Y != y) {
        VerificarEmergencia();
        if (posicao_Y < y) { AcionamentoMotorY(1); posicao_Y++; }
        else { AcionamentoMotorY(0); posicao_Y--; }
    }

    while (posicao_Z > z) {
        VerificarEmergencia();
        if (FdC_Z_Min == 0) {
        lcd.cls(); 
        lcd.printf("Fim de curso Z atingido");
        break;
    }
    AcionamentoMotorZ(2);
    posicao_Z--;
    }
}

//Inicialização da função MAIN() 
int main() {
    lcd.setCursor(TextLCD::CurOff_BlkOn);
    lcd.setBacklight(TextLCD::LightOn);
    encoderA.rise(&encoderSubir);

    Pipeta=1; // Garante que a pipeta está no estado "vazio" (pronta para sugar)

    inicio:

    // Reset de variáveis
    referenciado_X = false;
    referenciado_Y = false;
    referenciado_Z = false;
    posicao_de_coleta_salva = false;
    posicao_X = 0;
    posicao_Y = 0;
    posicao_Z = 0;
    encoder_val = 0;
    num_pontos_lib = 0;

    lcd.cls();
    lcd.locate(0, 0); lcd.printf("Deseja referenciar?");
    lcd.locate(0, 1); lcd.printf("Pressione INPUT");

    while (BotaoEncoder == 0);
    wait_ms(300);
    while (BotaoEncoder == 1);

    ReferenciarZ();
    ReferenciarX();
    ReferenciarY();

    lcd.cls();
    lcd.locate(0, 0); lcd.printf("Selecione coleta");
    lcd.locate(0, 1); lcd.printf("Pressione INPUT");

    while (!posicao_de_coleta_salva) {
        VerificarEmergencia();
        float leituraX = JoyX.read();
        float leituraY = JoyY.read();

        if (leituraY > 0.6f) { AcionamentoMotorY(1); posicao_Y++; }
        else if (leituraY < 0.4f) { AcionamentoMotorY(0); posicao_Y--; }

        if (leituraX > 0.6f) { AcionamentoMotorX(1); posicao_X++; }
        else if (leituraX < 0.4f) { AcionamentoMotorX(0); posicao_X--; }

        if (BotaoZP == 0 && FdC_Z_Max == 1) { 
            AcionamentoMotorZ(1); 
            posicao_Z++; 
        } 
        else if (BotaoZN == 0 && FdC_Z_Min == 1) { 
            AcionamentoMotorZ(2); 
            posicao_Z--; 
        }

        if (BotaoEncoder == 1) {
            wait_ms(300);
            SalvarPosicaoCOLETA();
            while (BotaoEncoder == 1);
            wait_ms(100);
        }
    }

    lcd.cls();
    lcd.locate(0, 0); lcd.printf("Quantos pontos");
    lcd.locate(0, 1); lcd.printf("de deposito?");

     encoder_val = 1;

    while (true) {
        VerificarEmergencia();
        lcd.locate(0, 2); lcd.printf("Selecionado: %d ", encoder_val);
        wait_ms(300);
        if (BotaoEncoder == 1) {
            num_pontos_lib = encoder_val;
            while (BotaoEncoder == 1);
            break;
        }
    }

    for (int i = 0; i < num_pontos_lib; i++) {
        lcd.cls();
        lcd.locate(0, 0); lcd.printf("Ponto %d: selecione", i + 1);

        while (true) {
            float leituraX = JoyX.read();
            float leituraY = JoyY.read();

            if (leituraY > 0.6f) { AcionamentoMotorY(1); posicao_Y++; }
            else if (leituraY < 0.4f) { AcionamentoMotorY(0); posicao_Y--; }

            if (leituraX > 0.6f) { AcionamentoMotorX(1); posicao_X++; }
            else if (leituraX < 0.4f) { AcionamentoMotorX(0); posicao_X--; }

            if (BotaoZP == 0 && FdC_Z_Max == 1) { 
                AcionamentoMotorZ(1); 
                posicao_Z++; 
            } 
            else if (BotaoZN == 0 && FdC_Z_Min == 1) { 
                AcionamentoMotorZ(2); 
                posicao_Z--; 
            }


            if (BotaoEncoder == 1) {
                posicoes_X[i] = posicao_X;
                posicoes_Y[i] = posicao_Y;
                posicoes_Z[i] = posicao_Z;

                lcd.cls();
                lcd.locate(0, 0); lcd.printf("X: %.1f Y: %.1f", passosParaMM(posicao_X), passosParaMM(posicao_Y));
                lcd.locate(0, 1); lcd.printf("Z: %.1f mm", passosParaMM(posicao_Z));
                wait_ms(2000);
                while (BotaoEncoder == 1);
                break;
            }
        }

        lcd.cls();
        lcd.locate(0, 0); lcd.printf("Qnts mls no ponto %d?", i + 1);
        encoder_val = 1;

        while (true) {
            VerificarEmergencia();
            lcd.locate(0, 1); lcd.printf("Selecionado: %d ml ", encoder_val);
            wait_ms(300);
            if (BotaoEncoder == 1) {
                volumes[i] = encoder_val;
                while (BotaoEncoder == 1);
                break;
            }
        }
    }

    lcd.cls();
    lcd.printf("Iniciando fluxo...");
    wait_ms(2000);


    for (int i = 0; i < num_pontos_lib; i++) {
        int viagens = volumes[i];
        for (int v = 0; v < viagens; v++) {

            VerificarEmergencia();
            lcd.cls(); 
            lcd.locate(0,0); lcd.printf("Indo extrair");
            lcd.locate(0,1); lcd.printf("do ponto de coleta");
            
            MoverPara(posicao_coletaX, posicao_coletaY, posicao_coletaZ);
            VerificarEmergencia();
            wait_ms(300);
            AcionarPipeta_Toggle(); // sugar
            wait_ms(1000);           // espera antes de subir Z novamente

            lcd.cls(); lcd.printf("Indo para Ponto %d", i+1);
            MoverPara(posicoes_X[i], posicoes_Y[i], posicoes_Z[i]);
            VerificarEmergencia();
            wait_ms(300);
            AcionarPipeta_Toggle(); // extrair
            wait_ms(1000);           // espera antes de subir Z novamente
    }
}

    lcd.cls();
    lcd.printf("Pipetagem OK!");
    wait_ms(1000);

    ElevarZ_AteTopo();

    // Exibe tela de retorno
    lcd.cls();
    lcd.locate(0, 0); lcd.printf("Aperte INPUT p/");
    lcd.locate(0, 1); lcd.printf("recomecar");

    // Aguarda o usuário apertar e soltar o botão corretamente
    while (BotaoEncoder == 1); // espera pressionar
    wait_ms(300);
    while (BotaoEncoder == 0); // espera soltar
    wait_ms(300);

    // Reinicia o fluxo
    goto inicio;

}