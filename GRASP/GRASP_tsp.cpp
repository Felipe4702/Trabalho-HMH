#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <random>
#include <limits>
#include <chrono>
#include <iomanip>

using namespace std;

struct Cidade {
    int id;
    int x;
    int y;
};

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

int avaliar_tsp(const vector<int>& caminho, const vector<vector<int>>& matriz) {
    int custo = 0, n = caminho.size();
    for (int i = 0; i < n - 1; i++) {
        custo += matriz[caminho[i]][caminho[i+1]];
    }
    custo += matriz[caminho[n-1]][caminho[0]];
    return custo;
}

vector<int> construcao_grasp(const vector<vector<int>>& matriz, double alpha, mt19937& gen) {
    int n = matriz.size();
    vector<int> solucao;
    vector<bool> visitado(n, false);
    
    uniform_int_distribution<> disInt(0, n - 1);
    int cidade_atual = disInt(gen);
    
    solucao.push_back(cidade_atual);
    visitado[cidade_atual] = true;
    
    while ((int)solucao.size() < n) {
        int c_min = numeric_limits<int>::max();
        int c_max = numeric_limits<int>::min();
        
        for (int i = 0; i < n; i++) {
            if (!visitado[i]) {
                int custo = matriz[cidade_atual][i];
                if (custo < c_min) c_min = custo;
                if (custo > c_max) c_max = custo;
            }
        }
        
        double limiar = c_min + alpha * (c_max - c_min);
        
        vector<int> LRC;
        for (int i = 0; i < n; i++) {
            if (!visitado[i] && matriz[cidade_atual][i] <= limiar) {
                LRC.push_back(i);
            }
        }
        
        if (LRC.empty()) {
            for (int i = 0; i < n; i++) {
                if (!visitado[i]) LRC.push_back(i);
            }
        }
        
        uniform_int_distribution<> disLRC(0, (int)LRC.size() - 1);
        int escolhida = LRC[disLRC(gen)];
        
        solucao.push_back(escolhida);
        visitado[escolhida] = true;
        cidade_atual = escolhida;
    }
    
    return solucao;
}

vector<int> busca_local(vector<int> solucao, const vector<vector<int>>& matriz) {
    bool melhorou = true;
    int custo_atual = avaliar_tsp(solucao, matriz);
    int n = solucao.size();
    
    while (melhorou) {
        melhorou = false;
        for (int i = 0; i < n - 1; i++) {
            for (int j = i + 1; j < n; j++) {
                reverse(solucao.begin() + i, solucao.begin() + j + 1);
                int custo_vizinho = avaliar_tsp(solucao, matriz);
                
                if (custo_vizinho < custo_atual) {
                    custo_atual = custo_vizinho;
                    melhorou = true;
                    break; 
                } else {
                    reverse(solucao.begin() + i, solucao.begin() + j + 1);
                }
            }
            if (melhorou) break;
        }
    }
    return solucao;
}

struct ResultadoExecucao {
    int execucao;
    int custo;
    double tempo_ms;
};

void executarGraspTSP(const vector<vector<int>>& matriz, int GRASPMax, double alpha, int config_id) {
    vector<ResultadoExecucao> resultados(10);
    double tempo_total = 0;

    random_device rd;
    mt19937 gen(rd());

    auto inicio_geral = chrono::high_resolution_clock::now();

    for (int exec = 0; exec < 10; exec++) {
        auto inicio_exec = chrono::high_resolution_clock::now();

        int custo_best_exec = numeric_limits<int>::max();

        for (int iter = 0; iter < GRASPMax; iter++) {
            vector<int> s = construcao_grasp(matriz, alpha, gen);
            s = busca_local(s, matriz);
            int custo_s = avaliar_tsp(s, matriz);
            
            if (custo_s < custo_best_exec) {
                custo_best_exec = custo_s;
            }
        }

        auto fim_exec = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> duracao = fim_exec - inicio_exec;

        resultados[exec].execucao = exec + 1;
        resultados[exec].custo = custo_best_exec;
        resultados[exec].tempo_ms = duracao.count();
        tempo_total += resultados[exec].tempo_ms;
    }

    auto fim_geral = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duracao_total = fim_geral - inicio_geral;

    // Estatísticas (Melhor, Pior, Média)
    int melhor = resultados[0].custo;
    int pior = resultados[0].custo;
    double soma_custos = 0;

    for (int i = 0; i < 10; i++) {
        if (resultados[i].custo < melhor) melhor = resultados[i].custo;
        if (resultados[i].custo > pior) pior = resultados[i].custo;
        soma_custos += resultados[i].custo;
    }
    double media = soma_custos / 10.0;

    // Exibição da Tabela de Resultados do GRASP TSP
    cout << "\n===================================================================\n";
    cout << " TABELA DE RESULTADOS (TSP - GRASP) - CONFIGURACAO " << config_id << "\n";
    cout << " (Alpha = " << alpha << ", GRASPMax = " << GRASPMax << " iterecoes)\n";
    cout << "===================================================================\n";
    cout << left << setw(10) << "Execucao" << setw(15) << "Custo (Obj)" << setw(15) << "Tempo (ms)" << endl;
    cout << "-------------------------------------------------------------------\n";
    for (int i = 0; i < 10; i++) {
        cout << left << setw(10) << resultados[i].execucao 
             << setw(15) << resultados[i].custo 
             << setw(15) << fixed << setprecision(2) << resultados[i].tempo_ms << endl;
    }
    cout << "-------------------------------------------------------------------\n";
    cout << " ESTATISTICAS FINAIS E TEMPO TOTAL\n";
    cout << "-------------------------------------------------------------------\n";
    cout << left << setw(15) << "Melhor Custo" << setw(15) << "Pior Custo" << setw(15) << "Media Custos" << setw(20) << "Tempo Total (ms)" << endl;
    cout << left << setw(15) << melhor 
         << setw(15) << pior 
         << setw(15) << fixed << setprecision(2) << media 
         << setw(20) << duracao_total.count() << endl;
    cout << "===================================================================\n";
}

int main() {
    vector<int> dados = extrairDadosLimpos("cidadestsp.txt");

    if (dados.empty() || dados.size() % 3 != 0) {
        cout << "Erro: Arquivo vazio ou formato inválido." << endl;
        return 1;
    }

    int numVertices = dados.size() / 3;
    vector<Cidade> cidades(numVertices);
    
    int index = 0;
    for (int i = 0; i < numVertices; ++i) {
        cidades[i].id = dados[index];
        cidades[i].x = dados[index + 1];
        cidades[i].y = dados[index + 2];
        index += 3;
    }

    vector<vector<int>> matriz(numVertices, vector<int>(numVertices, 0));
    for (int i = 0; i < numVertices; ++i) {
        for (int j = 0; j < numVertices; ++j) {
            if (i != j) {
                double dx = cidades[i].x - cidades[j].x;
                double dy = cidades[i].y - cidades[j].y;
                matriz[i][j] = round(sqrt(dx * dx + dy * dy));
            }
        }
    }

    // Configuração 1 de Parâmetros
    executarGraspTSP(matriz, 100, 0.2, 1);

    // Configuração 2 de Parâmetros (Alterando Alpha e Critério de Parada / iterações)
    executarGraspTSP(matriz, 200, 0.5, 2);

    return 0;
}