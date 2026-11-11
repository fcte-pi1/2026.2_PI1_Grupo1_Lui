#ifndef SENSORES_TOF_H
#define SENSORES_TOF_H

#include <stdint.h>
#include "../sensores/ClassificadorParedes.h"  // AmostraParedes
#include "../sensores/LeituraToF.h"
#include "../app/InterfacesHardware.h"  // ISensoresToF

// Driver dos três VL53L0X (porte de hw/firm/src/vl53l0x_manager.cpp com os pinos e
// endereços de hardware/Pinos.h), com leitura SEM ESPERA (achado B10).
//
// O hw/firm lê com readRangeContinuousMillimeters(), que espera a amostra ficar pronta: até
// 33 ms por sensor normalmente e até 500 ms por sensor se um deles parar de responder. Aqui
// atualizar() só consulta o registrador de interrupção de cada sensor; se houver amostra
// pronta, lê a distância e libera o sensor, senão segue em frente. Quem decide se a leitura
// ainda vale é a LeituraToF (100 ms sem amostra nova = inválida), testada no nativo.
//
// Uso: iniciar() no setup(); atualizar(agoraMs) em todo tick (o próprio driver limita as
// consultas a uma a cada TOF_PERIODO_CONSULTA_MS); depois, esquerdo()/frontal()/direito()
// para o controle de reta e amostraParedes() para o ClassificadorParedes.
class SensoresToF : public ISensoresToF {
public:
    // Liga os sensores um a um pelos XSHUT, atribui os endereços e começa a medição
    // contínua. Retorna true se os três responderam; um sensor que falhar fica sempre
    // inválido e o resto continua funcionando.
    bool iniciar();

    void atualizar(uint32_t agoraMs) override;

    LeituraToF& esquerdo() override { return esq_; }
    LeituraToF& frontal() override { return frente_; }
    LeituraToF& direito() override { return dir_; }

    AmostraParedes amostraParedes(uint32_t agoraMs) const {
        return amostraDasLeituras(esq_, frente_, dir_, agoraMs);
    }

    // Sensor respondeu na inicialização (para o log e o autoteste).
    bool esquerdoIniciado() const { return iniciado_[0]; }
    bool frontalIniciado() const { return iniciado_[1]; }
    bool direitoIniciado() const { return iniciado_[2]; }

private:
    LeituraToF esq_, frente_, dir_;
    bool iniciado_[3] = {false, false, false};
    uint32_t esperaAteMs_[3] = {0, 0, 0};  // sensor com erro de I2C fica sem consulta até lá
    uint32_t ultimaConsultaMs_ = 0;
    bool consultou_ = false;
};

#endif
