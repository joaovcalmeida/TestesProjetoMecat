// #include "mbed.h"
// #include "TextLCD.h"

// #define velo 0.01 // tempo entre os passos
// #define PASSO_MM 1.25f // 1 passo = 1.25mm

// // LCD (RS, E, D4, D5, D6, D7)
// TextLCD lcd(PA_5, PA_6, PA_7, PA_8, PA_9, PA_10);

// // EIXOS X e Y com DRIVER (Step/Dir)
// DigitalOut StepX(D10);
// DigitalOut DirX(D11);
// DigitalOut StepY(D6);
// DigitalOut DirY(D7);

// // EIXO Z com controle via MOSFET (sequenciamento de bobinas)
// BusOut MotorZ(D2, D3, D4, D5);

// // Botões
// DigitalIn BotaoXP(A3);
// DigitalIn BotaoXN(A4);
// DigitalIn BotaoYP(A1);
// DigitalIn BotaoYN(A2);
// DigitalIn BotaoZP(A0);
// DigitalIn BotaoZN(PC_13);
// DigitalIn BotaoInput(PB_12, PullUp);

// // Sensores de fim de curso
// DigitalIn FdC_Z_Min(D10);
// DigitalIn FdC_X_Min(PB_1);
// DigitalIn FdC_Y_Min(PB_15);

// // Variáveis globais
// int posicao_X = 0, posicao_Y = 0, posicao_Z = 0;
// bool referenciado_X = false, referenciado_Y = false, referenciado_Z = false;
// int estado_X = 0, estado_Y = 0, estado_Z = 0;

// const int tamanho_array = 100;
// int posicoes_X[tamanho_array], posicoes_Y[tamanho_array], posicoes_Z[tamanho_array];
// int num_posicoes_salvas = 0;
// bool posicao_de_coleta_salva = false;
// int posicao_coletaX = 0, posicao_coletaY = 0, posicao_coletaZ = 0;

// // ===== Utilitários =====
// float passosParaMM(int passos) {
//     return passos * PASSO_MM;
// }

// void pulsoStep(DigitalOut &step) {
//     step = 1;
//     wait_us(500);
//     step = 0;
//     wait_us(500);
// }

// // ===== Acionamento =====
// void AcionamentoMotorX(int estado) {
//     if (estado == 1) { DirX = 1; pulsoStep(StepX); posicao_X++; }
//     else if (estado == 2) { DirX = 0; pulsoStep(StepX); posicao_X--; }
// }

// void AcionamentoMotorY(int estado) {
//     if (estado == 1) { DirY = 1; pulsoStep(StepY); posicao_Y++; }
//     else if (estado == 2) { DirY = 0; pulsoStep(StepY); posicao_Y--; }
// }

// void AcionamentoMotorZ(int estado) {
//     int f[4] = {0x01, 0x02, 0x04, 0x08};
//     if (estado == 1) {
//         for (int i = 0; i < 4; i++) { MotorZ = f[i]; wait(velo); }
//         posicao_Z++;
//     } else if (estado == 2) {
//         for (int i = 3; i >= 0; i--) { MotorZ = f[i]; wait(velo); }
//         posicao_Z--;
//     } else {
//         MotorZ = 0;
//     }
// }

// // ===== Referenciamento =====
// void ReferenciarX() {
//     if (referenciado_X) return;
//     switch (estado_X) {
//         case 0:
//             if (FdC_X_Min == 1) AcionamentoMotorX(1);
//             else { estado_X = 1; }
//             break;
//         case 1:
//             AcionamentoMotorX(2);
//             if (FdC_X_Min == 1) {
//                 posicao_X = 0;
//                 referenciado_X = true;
//                 lcd.cls(); lcd.printf("X referenciado\nX=0");
//             }
//             break;
//     }
// }

// void ReferenciarY() {
//     if (referenciado_Y) return;
//     switch (estado_Y) {
//         case 0:
//             if (FdC_Y_Min == 1) AcionamentoMotorY(1);
//             else { estado_Y = 1; }
//             break;
//         case 1:
//             AcionamentoMotorY(2);
//             if (FdC_Y_Min == 1) {
//                 posicao_Y = 0;
//                 referenciado_Y = true;
//                 lcd.cls(); lcd.printf("Y referenciado\nY=0");
//             }
//             break;
//     }
// }

// void ReferenciarZ() {
//     if (referenciado_Z) return;
//     switch (estado_Z) {
//         case 0:
//             if (FdC_Z_Min == 1) AcionamentoMotorZ(1);
//             else { MotorZ = 0; estado_Z = 1; }
//             break;
//         case 1:
//             AcionamentoMotorZ(2);
//             if (FdC_Z_Min == 1) {
//                 MotorZ = 0;
//                 posicao_Z = 0;
//                 referenciado_Z = true;
//                 lcd.cls(); lcd.printf("Z referenciado\nZ=0");
//             }
//             break;
//     }
// }

// // ===== Salvamento de posições =====
// void SalvarPosicaoCOLETA() {
//     posicao_coletaX = posicao_X;
//     posicao_coletaY = posicao_Y;
//     posicao_coletaZ = posicao_Z;
//     posicao_de_coleta_salva = true;

//     lcd.cls();
//     lcd.printf("Coleta salva:\nX=%.1f Y=%.1f\nZ=%.1f", passosParaMM(posicao_coletaX), passosParaMM(posicao_coletaY), passosParaMM(posicao_coletaZ));
//     wait_ms(3000);
//     lcd.cls();
// }

// void SalvarPosicaoLiberacao() {
//     if (num_posicoes_salvas < tamanho_array) {
//         posicoes_X[num_posicoes_salvas] = posicao_X;
//         posicoes_Y[num_posicoes_salvas] = posicao_Y;
//         posicoes_Z[num_posicoes_salvas] = posicao_Z;
//         num_posicoes_salvas++;

//         lcd.cls();
//         lcd.printf("Lib %d salva\nX=%.1f Y=%.1f Z=%.1f", num_posicoes_salvas, passosParaMM(posicao_X), passosParaMM(posicao_Y), passosParaMM(posicao_Z));
//         wait_ms(2000);
//         lcd.cls();
//     }
// }

// // ===== Movimento automático =====
// void MoverParaPosicaoFornecida(int alvo_X, int alvo_Y, int alvo_Z) {
//     lcd.cls();
//     lcd.printf("Movendo para:\nX=%.1f Y=%.1f\nZ=%.1f", passosParaMM(alvo_X), passosParaMM(alvo_Y), passosParaMM(alvo_Z));

//     while (posicao_X != alvo_X) {
//         if (posicao_X < alvo_X) { AcionamentoMotorX(1); }
//         else { AcionamentoMotorX(2); }
//     }
//     while (posicao_Y != alvo_Y) {
//         if (posicao_Y < alvo_Y) { AcionamentoMotorY(1); }
//         else { AcionamentoMotorY(2); }
//     }
//     while (posicao_Z != alvo_Z) {
//         if (posicao_Z < alvo_Z) { AcionamentoMotorZ(1); }
//         else { AcionamentoMotorZ(2); }
//     }
//     lcd.locate(0, 3); lcd.printf("OK"); wait_ms(1000);
//     lcd.cls();
// }

// // ===== MAIN =====
// int main() {
//     while (!referenciado_X) ReferenciarX();
//     while (!referenciado_Y) ReferenciarY();
//     while (!referenciado_Z) ReferenciarZ();

//     while (true) {
//         if (BotaoXP == 0) AcionamentoMotorX(1);
//         else if (BotaoXN == 0) AcionamentoMotorX(2);

//         if (BotaoYP == 0) AcionamentoMotorY(1);
//         else if (BotaoYN == 0) AcionamentoMotorY(2);

//         if (BotaoZP == 0) AcionamentoMotorZ(1);
//         else if (BotaoZN == 0) AcionamentoMotorZ(2);

//         if (BotaoInput == 0) {
//             wait_ms(300);
//             if (!posicao_de_coleta_salva) SalvarPosicaoCOLETA();
//             else SalvarPosicaoLiberacao();
//             while (BotaoInput == 0);
//             wait_ms(100);
//         }
//     }
// }
