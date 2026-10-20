#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <random>
#include <limits>
#include <chrono>
#include <iomanip>

using namespace std;

struct ItemMochila {
    int id;
    int peso;
    int valor;
};

// Função reaproveitada para limpeza e leitura do arquivo
vector<int> extrairDadosLimpos(const string& nomeArquivo) {
    vector<int> dados;
    ifstream arquivo(nomeArquivo);
    string linha;
    if (!arquivo.is_open()) return dados; 
    
    while (getline(arquivo, linha)) {
        size_t pos = linha.find('#');
        if (pos != string::npos) linha = linha.substr(0, pos);
        
        stringstream ss(linha);
        int valor;
        while (ss >> valor) dados.push_back(valor);
    }
    arquivo.close();
    return dados;
}

// Avaliação da mochila (retorna o valor total ou 0 se estourar a capacidade)
int avaliar_mochila(const vector<int>& solucao, const vector<ItemMochila>& itens, int cap, int& peso_total_out) {
    int peso = 0;
    int valor = 0;
    for (size_t i = 0; i < solucao.size(); i++) {
        if (solucao[i] == 1) {
            peso += itens[i].peso;
            valor += itens[i].valor;
        }
    }
    peso_total_out = peso;
    if (peso > cap) return 0; // Inviável
    return valor;
}

// Construtiva GRASP para a Mochila baseada na razão Valor/Peso (Guloso-Randomizado)
vector<int> construcao_grasp_mochila(const vector<ItemMochila>& itens, int cap, double alpha, mt19937& gen) {
    int n = itens.size();
    vector<int> solucao(n, 0);
    vector<bool> disponivel(n, true);
    int peso_atual = 0;

    while (true) {
        // Encontra itens viáveis que ainda não foram colocados
        vector<int> candidatos;
        vector<double> razoes;
        double r_min = numeric_limits<double>::max();
        double r_max = numeric_limits<double>::min();

        for (int i = 0; i < n; i++) {
            if (disponivel[i] && (peso_atual + itens[i].peso <= cap)) {
                double razao = (double)itens[i].valor / itens[i].peso;
                candidatos.push_back(i);
                razoes.push_back(razao);
                if (razao < r_min) r_min = razao;
                if (razao > r_max) r_max = razao;
            }
        }

        // Se não houver mais itens que caibam, encerra a construção
        if (candidatos.empty()) break;

        // Limiar da LRC (Lista Restrita de Candidatos) para maximização (razão alta)
        double limiar = r_max - alpha * (r_max - r_min);

        vector<int> LRC;
        for (size_t i = 0; i < candidatos.size(); i++) {
            if (razoes[i] >= limiar) {
                LRC.push_back(candidatos[i]);
            }
        }

        // Sorteia um item da LRC
        uniform_int_distribution<> disLRC(0, (int)LRC.size() - 1);
        int escolhido = LRC[disLRC(gen)];

        solucao[escolhido] = 1;
        peso_atual += itens[escolhido].peso;
        disponivel[escolhido] = false;
    }

    return solucao;
}

// Busca local simples por inversão de bits (flip) para tentar adicionar/remover itens e melhorar o valor
vector<int> busca_local_mochila(vector<int> solucao, const vector<ItemMochila>& itens, int cap) {
    int dummy_peso = 0;
    int custo_atual = avaliar_mochila(solucao, itens, cap, dummy_peso);
    int n = solucao.size();
    bool melhorou = true;

    while (melhorou) {
        melhorou = false;
        for (int i = 0; i < n; i++) {
            solucao[i] = 1 - solucao[i]; // inverte o bit (coloca ou retira o item)
            int peso_viz = 0;
            int custo_vizinho = avaliar_mochila(solucao, itens, cap, peso_viz);

            if (custo_vizinho > custo_atual) {
                custo_atual = custo_vizinho;
                melhorou = true;
                break;
            } else {
                solucao[i] = 1 - solucao[i]; // desfaz a alteração se não melhorou
            }
        }
    }
    return solucao;
}

struct ResultadoExecucaoMochila {
    int execucao;
    int valor_obj;
    int peso_total;
    double tempo_ms;
};

void executarGraspMochila(const vector<ItemMochila>& itens, int cap, int GRASPMax, double alpha, int config_id) {
    vector<ResultadoExecucaoMochila> resultados(10);
    double tempo_total = 0;

    random_device rd;
    mt19937 gen(rd());

    auto inicio_geral = chrono::high_resolution_clock::now();

    for (int exec = 0; exec < 10; exec++) {
        auto inicio_exec = chrono::high_resolution_clock::now();

        int melhor_valor_exec = -1;
        int melhor_peso_exec = 0;

        for (int iter = 0; iter < GRASPMax; iter++) {
            vector<int> s = construcao_grasp_mochila(itens, cap, alpha, gen);
            s = busca_local_mochila(s, itens, cap);
            
            int peso_s = 0;
            int valor_s = avaliar_mochila(s, itens, cap, peso_s);

            if (valor_s > melhor_valor_exec) {
                melhor_valor_exec = valor_s;
                melhor_peso_exec = peso_s;
            }
        }

        auto fim_exec = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> duracao = fim_exec - inicio_exec;

        resultados[exec].execucao = exec + 1;
        resultados[exec].valor_obj = melhor_valor_exec;
        resultados[exec].peso_total = melhor_peso_exec;
        resultados[exec].tempo_ms = duracao.count();
        tempo_total += resultados[exec].tempo_ms;
    }

    auto fim_geral = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duracao_total = fim_geral - inicio_geral;

    // Estatísticas (Melhor, Pior, Média) - Para mochila buscamos o MAIOR valor
    int melhor = resultados[0].valor_obj;
    int pior = resultados[0].valor_obj;
    double soma_valores = 0;

    for (int i = 0; i < 10; i++) {
        if (resultados[i].valor_obj > melhor) melhor = resultados[i].valor_obj;
        if (resultados[i].valor_obj < pior) pior = resultados[i].valor_obj;
        soma_valores += resultados[i].valor_obj;
    }
    double media = soma_valores / 10.0;

    // Exibição da Tabela de Resultados da Mochila GRASP
    cout << "\n===================================================================\n";
    cout << " TABELA DE RESULTADOS (MOCHILA - GRASP) - CONFIGURACAO " << config_id << "\n";
    cout << " (Alpha = " << alpha << ", GRASPMax = " << GRASPMax << " iteracoes)\n";
    cout << "===================================================================\n";
    cout << left << setw(10) << "Execucao" << setw(15) << "Valor (Obj)" << setw(15) << "Peso Total" << setw(15) << "Tempo (ms)" << endl;
    cout << "-------------------------------------------------------------------\n";
    for (int i = 0; i < 10; i++) {
        cout << left << setw(10) << resultados[i].execucao 
             << setw(15) << resultados[i].valor_obj 
             << setw(15) << resultados[i].peso_total 
             << setw(15) << fixed << setprecision(2) << resultados[i].tempo_ms << endl;
    }
    cout << "-------------------------------------------------------------------\n";
    cout << " ESTATISTICAS FINAIS E TEMPO TOTAL\n";
    cout << "-------------------------------------------------------------------\n";
    cout << left << setw(15) << "Melhor Valor" << setw(15) << "Pior Valor" << setw(15) << "Media Valores" << setw(20) << "Tempo Total (ms)" << endl;
    cout << left << setw(15) << melhor 
         << setw(15) << pior 
         << setw(15) << fixed << setprecision(2) << media 
         << setw(20) << duracao_total.count() << endl;
    cout << "===================================================================\n";
}


int main() {
    vector<int> dados = extrairDadosLimpos("mochila_4_20");

    if (dados.size() < 2) {
        cout << "Erro: Arquivo vazio ou formato invalido." << endl;
        return 1;
    }

    int numItens = dados[0];
    int capacidade = dados[1];

    vector<ItemMochila> itens;
    int index = 2;

    for (int i = 0; i < numItens && index + 1 < dados.size(); i++) {
        ItemMochila item;
        item.id = i + 1;
        item.valor = dados[index];
        item.peso = dados[index + 1];
        itens.push_back(item);
        index += 2;
    }

    // Configuracao 1 de Parametros
    executarGraspMochila(itens, capacidade, 100, 0.2, 1);

    // Configuracao 2 de Parametros
    executarGraspMochila(itens, capacidade, 200, 0.4, 2);


    vector<int> dados2 = extrairDadosLimpos("mochila_100_1000_1");

    if (dados2.size() < 2) {
        cout << "Erro: Arquivo vazio ou formato invalido." << endl;
        return 1;
    }

    int numItens2 = dados2[0];
    int capacidade2 = dados2[1];

    vector<ItemMochila> itens2;
    int index2 = 2;

    for (int i = 0; i < numItens2 && index2 + 1 < dados2.size(); i++) {
        ItemMochila item;
        item.id = i + 1;
        item.valor = dados2[index2];
        item.peso = dados2[index2 + 1];
        itens2.push_back(item);
        index2 += 2;
    }

    // Configuracao 1 de Parametros
    executarGraspMochila(itens2, capacidade2, 100, 0.2, 1);

    // Configuracao 2 de Parametros
    executarGraspMochila(itens2, capacidade2, 200, 0.4, 2);

    return 0;
}
