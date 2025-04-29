// #include "mbed.h"
// #include "TextLCD.h"

// #define velo 0.01 // tempo entre os passos
// #define PASSO_MM 1.25f // 1 passo = 1.25mm

// // LCD (RS, E, D4, D5, D6, D7)
// TextLCD lcd(PA_5, PA_6, PA_7, PA_8, PA_9, PA_10);

// // Motores e botões
// BusOut MotorX(D10, D11, D12, D13);
// BusOut MotorY(D6, D7, D8, D9);
// BusOut MotorZ(D2, D3, D4, D5);

// // Botões de movimentação e input
// DigitalIn BotaoXP(A3);
// DigitalIn BotaoXN(A4);
// DigitalIn BotaoYP(A1);
// DigitalIn BotaoYN(A2);
// DigitalIn BotaoZP(A0);
// DigitalIn BotaoZN(PC_13);
// DigitalIn BotaoInput(PB_12, PullUp); // botão para salvar posição

// // Sensores de fim de curso
// DigitalIn FdC_Z_Min(D10);
// DigitalIn FdC_Z_Max(D11);
// DigitalIn FdC_X_Max(PB_2);
// DigitalIn FdC_X_Min(PB_1);
// DigitalIn FdC_Y_Max(PB_14);
// DigitalIn FdC_Y_Min(PB_15);

// // Variáveis de posição (em passos)
// int posicao_X = 0, posicao_Y = 0, posicao_Z = 0;
// int estado_Z = 0, estado_X = 0, estado_Y = 0;
// bool referenciado_Z = false, referenciado_X = false, referenciado_Y = false;

// // Vetores e controle de posições salvas
// const int tamanho_array = 100;
// int posicoes_X[tamanho_array], posicoes_Y[tamanho_array], posicoes_Z[tamanho_array];
// int num_posicoes_salvas = 0;
// bool posicao_de_coleta_salva = false;
// int posicao_coletaX = 0, posicao_coletaY = 0, posicao_coletaZ = 0;

// // Prototipagem de funções
// void AcionamentoMotorX(int estado);
// void AcionamentoMotorY(int estado);
// void AcionamentoMotorZ(int estado);

// // Funções de referenciamento para cada eixo
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

// void ReferenciarX() {
//     if (referenciado_X) return;
//     switch (estado_X) {
//         case 0:
//             if (FdC_X_Min == 1) AcionamentoMotorX(1);
//             else { MotorX = 0; estado_X = 1; }
//             break;
//         case 1:
//             AcionamentoMotorX(2);
//             if (FdC_X_Min == 1) {
//                 MotorX = 0;
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
//             else { MotorY = 0; estado_Y = 1; }
//             break;
//         case 1:
//             AcionamentoMotorY(2);
//             if (FdC_Y_Min == 1) {
//                 MotorY = 0;
//                 posicao_Y = 0;
//                 referenciado_Y = true;
//                 lcd.cls(); lcd.printf("Y referenciado\nY=0");
//             }
//             break;
//     }
// }

// // Converte passos para milímetros
// float passosParaMM(int passos) {
//     return passos * PASSO_MM;
// }

// // Salva a posição atual como posição de coleta
// void SalvarPosicaoCOLETA() {
//     posicao_coletaX = posicao_X;
//     posicao_coletaY = posicao_Y;
//     posicao_coletaZ = posicao_Z;
//     posicao_de_coleta_salva = true;

//     lcd.cls();
//     lcd.locate(0, 0); lcd.printf("Coleta salva:");
//     lcd.locate(0, 1); lcd.printf("X = %.1f mm", passosParaMM(posicao_coletaX));
//     lcd.locate(0, 2); lcd.printf("Y = %.1f mm", passosParaMM(posicao_coletaY));
//     lcd.locate(0, 3); lcd.printf("Z = %.1f mm", passosParaMM(posicao_coletaZ));
//     wait_ms(3000);
//     lcd.cls();
// }

// // Salva a posição atual como uma posição de liberação
// void SalvarPosicaoLiberacao() {
//     if (num_posicoes_salvas < tamanho_array) {
//         posicoes_X[num_posicoes_salvas] = posicao_X;
//         posicoes_Y[num_posicoes_salvas] = posicao_Y;
//         posicoes_Z[num_posicoes_salvas] = posicao_Z;

//         lcd.cls();
//         lcd.locate(0, 0); lcd.printf("Lib %d salva!", num_posicoes_salvas + 1);
//         lcd.locate(0, 1); lcd.printf("X: %.1f mm", passosParaMM(posicao_X));
//         lcd.locate(0, 2); lcd.printf("Y: %.1f mm", passosParaMM(posicao_Y));
//         lcd.locate(0, 3); lcd.printf("Z: %.1f mm", passosParaMM(posicao_Z));

//         num_posicoes_salvas++;

//         wait_ms(1000);
//         lcd.cls();
//         if (num_posicoes_salvas < tamanho_array) {
//             lcd.locate(0, 0); lcd.printf("Prox: pos %d", num_posicoes_salvas + 1);
//         } else {
//             lcd.locate(0, 0); lcd.printf("Pronto p/ pipetar");
//         }
//     } else {
//         lcd.cls();
//         lcd.locate(0, 0); lcd.printf("Limite atingido!");
//         lcd.locate(0, 1); lcd.printf("Max: %d posicoes", tamanho_array);
//         wait_ms(2000);
//         lcd.cls();
//     }
// }

// // Move o sistema até a posição fornecida
// void MoverParaPosicaoFornecida(int alvo_X, int alvo_Y, int alvo_Z) {
//     lcd.cls();
//     lcd.locate(0, 0); lcd.printf("Indo para:");
//     lcd.locate(0, 1); lcd.printf("X:%.1f Y:%.1f", passosParaMM(alvo_X), passosParaMM(alvo_Y));
//     lcd.locate(0, 2); lcd.printf("Z:%.1f mm", passosParaMM(alvo_Z));

//     while (posicao_X != alvo_X) {
//         if (posicao_X < alvo_X) { AcionamentoMotorX(1); posicao_X++; }
//         else { AcionamentoMotorX(2); posicao_X--; }
//     }
//     MotorX = 0; wait_ms(200);

//     while (posicao_Y != alvo_Y) {
//         if (posicao_Y < alvo_Y) { AcionamentoMotorY(1); posicao_Y++; }
//         else { AcionamentoMotorY(2); posicao_Y--; }
//     }
//     MotorY = 0; wait_ms(200);

//     while (posicao_Z != alvo_Z) {
//         if (posicao_Z < alvo_Z) { AcionamentoMotorZ(1); posicao_Z++; }
//         else { AcionamentoMotorZ(2); posicao_Z--; }
//     }
//     MotorZ = 0; wait_ms(200);

//     lcd.locate(0, 3); lcd.printf("Mov. finalizado!");
//     wait_ms(1000);
//     lcd.cls();
// }

// // Funções de acionamento dos motores com contagem de passos
// void AcionamentoMotorX(int estado) {
//     int f[4] = {0x01, 0x02, 0x04, 0x08};
//     switch (estado) {
//         case 1: for (int i = 0; i < 4; i++) { MotorX = f[i]; wait(velo); } posicao_X++; break;
//         case 2: for (int i = 3; i >= 0; i--) { MotorX = f[i]; wait(velo); } posicao_X--; break;
//         default: MotorX = 0; break;
//     }
// }

// void AcionamentoMotorY(int estado) {
//     int f[4] = {0x01, 0x02, 0x04, 0x08};
//     switch (estado) {
//         case 1: for (int i = 0; i < 4; i++) { MotorY = f[i]; wait(velo); } posicao_Y++; break;
//         case 2: for (int i = 3; i >= 0; i--) { MotorY = f[i]; wait(velo); } posicao_Y--; break;
//         default: MotorY = 0; break;
//     }
// }

// void AcionamentoMotorZ(int estado) {
//     int f[4] = {0x01, 0x02, 0x04, 0x08};
//     switch (estado) {
//         case 1: for (int i = 0; i < 4; i++) { MotorZ = f[i]; wait(velo); } posicao_Z++; break;
//         case 2: for (int i = 3; i >= 0; i--) { MotorZ = f[i]; wait(velo); } posicao_Z--; break;
//         default: MotorZ = 0; break;
//     }
// }

// // Loop principal do programa
// int main() {
//     while (!referenciado_Z) ReferenciarZ();
//     while (!referenciado_X) ReferenciarX();
//     while (!referenciado_Y) ReferenciarY();

//     while (true) {
//         // Controle manual dos eixos com botões físicos
//         if (BotaoXP == 0) AcionamentoMotorX(1);
//         else if (BotaoXN == 0) AcionamentoMotorX(2);
//         else AcionamentoMotorX(0);

//         if (BotaoYP == 0) AcionamentoMotorY(1);
//         else if (BotaoYN == 0) AcionamentoMotorY(2);
//         else AcionamentoMotorY(0);

//         if (BotaoZP == 0) AcionamentoMotorZ(1);
//         else if (BotaoZN == 0) AcionamentoMotorZ(2);
//         else AcionamentoMotorZ(0);

//         // Salvamento de posição ao pressionar BotaoInput
//         if (BotaoInput == 0) {
//             wait_ms(300);
//             if (!posicao_de_coleta_salva) SalvarPosicaoCOLETA();
//             else SalvarPosicaoLiberacao();
//             while (BotaoInput == 0);
//             wait_ms(100);
//         }
//     }
// }
