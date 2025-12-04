**Alunos:** Rafael Abrantes e Gustavo Fernandes

---

## 📋 Sobre o Projeto
Este projeto consiste em um sistema embarcado para controle de tráfego (veículos e pedestres) utilizando **RTOS (Threads e Mutex)**. O objetivo principal foi garantir a sincronização entre dois microcontroladores, validando tudo através do **Modelo V de testes**.

## ⚙️ Funcionalidades Implementadas

### 1. Semáforo de Veículos (3 LEDs)
* **Controle:** 3 Threads independentes (Verde, Amarelo, Vermelho).
* **Ciclo:** Verde (3s) -> Amarelo (1s) -> Vermelho (4s).
* **Sincronismo:** Opera coordenado com o semáforo de pedestres.

### 2. Semáforo de Pedestres (2 LEDs)
* **Controle:** 2 Threads independentes (Verde, Vermelho).
* **Ciclo:** Verde (4s) -> Vermelho (4s).
* **Segurança:** Uso de semáforo para impedir acionamento simultâneo indevido.

### 3. Modos Especiais
* **🌙 Modo Noturno:** LEDs piscam de forma intermitente (Amarelo para carros, Vermelho para pedestres) a cada 2 segundos.
* **🔘 Botão de Travessia:** Interrupção externa que fecha o sinal para carros e libera a passagem de pedestres com segurança.

## 🛠️ Detalhes Técnicos
* **Arquitetura:** Threads com proteção de recursos via Semáforo.
* **Hardware:** Sistema distribuído entre dois microcontroladores.
* **Linguagem:** C (Ambiente Embarcado).

## ✅ Validação e Testes (Modelo V)
O código foi validado seguindo o plano de testes:
1.  **Testes Unitários:** Verificação individual de cada thread/LED.

https://github.com/user-attachments/assets/307e63b0-c2af-4dd9-8df6-60a665680493


2.  **Testes de Integração:** Validação da comunicação e sincronia entre os dois semáforos.

https://github.com/user-attachments/assets/0d6b39bf-a534-44bd-8aea-cb7b7056ebda


3.  **Testes de Sistema:** Simulação completa dos modos Noturno e Botão de Travessia.

https://github.com/user-attachments/assets/e3d57b77-16b2-4770-85e3-c1b862808234


