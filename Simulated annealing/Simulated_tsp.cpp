#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <random>
#include <chrono>
#include <iomanip>

using namespace std;

struct Cidade {
    int id;
    int x;
    int y;
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

int avaliar_tsp(const vector<int>& caminho, const vector<vector<int>>& matriz) {
    int custo = 0, n = caminho.size();
    for (int i = 0; i < n - 1; i++) {
        if (matriz[caminho[i]][caminho[i+1]] == 0) return 999999;
        custo += matriz[caminho[i]][caminho[i+1]];
    }
    if (matriz[caminho[n-1]][caminho[0]] == 0) return 999999;
    return custo + matriz[caminho[n-1]][caminho[0]];
}

// Estrutura para armazenar os resultados de cada execução
struct ResultadoExecucao {
    int execucao;
    int custo;
    double tempo_ms;
};

void executarSimulatedAnnealingTSP(const vector<Cidade>& cidades, const vector<vector<int>>& matriz, double T_init, double T_min, int SAmax, double alpha, int config_id) {
    int n = cidades.size();
    vector<ResultadoExecucao> resultados(10);
    double tempo_total = 0;

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> disReal(0.0, 1.0);
    uniform_int_distribution<> disInt(0, n - 1);

    auto inicio_geral = chrono::high_resolution_clock::now();

    for (int exec = 0; exec < 10; exec++) {
        auto inicio_exec = chrono::high_resolution_clock::now();

        vector<int> s(n);
        for (int i = 0; i < n; i++) s[i] = i;
        shuffle(s.begin(), s.end(), gen);
        
        int custo_s = avaliar_tsp(s, matriz);

        vector<int> s_best = s;
        int custo_best = custo_s;

        double T = T_init;
        int iteracoes_T = 0;

        while (T > T_min) {
            while (iteracoes_T < SAmax) {
                iteracoes_T++;

                vector<int> s_vizinho = s;
                int pos1 = disInt(gen);
                int pos2 = disInt(gen);
                while (pos1 == pos2) pos2 = disInt(gen);
                swap(s_vizinho[pos1], s_vizinho[pos2]);
                
                int custo_vizinho = avaliar_tsp(s_vizinho, matriz);

                if (custo_vizinho < custo_s) {
                    s = s_vizinho;
                    custo_s = custo_vizinho;
                    
                    if (custo_vizinho < custo_best) {
                        s_best = s_vizinho;
                        custo_best = custo_vizinho;
                    }
                } 
                else {
                    double probabilidade = exp((custo_s - custo_vizinho) / T);
                    if (disReal(gen) < probabilidade) {  
                        s = s_vizinho;
                        custo_s = custo_vizinho;
                    }
                }
            }
            T = alpha * T;   
            iteracoes_T = 0; 
        }

        auto fim_exec = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> duracao = fim_exec - inicio_exec;

        resultados[exec].execucao = exec + 1;
        resultados[exec].custo = custo_best;
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
        if (resultados[i].custo < melhor) melhor = resultados[i].custo; // No TSP buscamos o menor custo
        if (resultados[i].custo > pior) pior = resultados[i].custo;
        soma_custos += resultados[i].custo;
    }
    double media = soma_custos / 10.0;

    // Exibição da Tabela exigida pelo trabalho
    cout << "\n===================================================================\n";
    cout << " TABELA DE RESULTADOS (TSP) - CONFIGURACAO " << config_id << "\n";
    cout << " (T0 = " << T_init << ", alpha = " << alpha << ", SAmax = " << SAmax << ")\n";
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
        cout << "Erro: Arquivo vazio, não encontrado ou com formato inválido (esperado: id x y)." << endl;
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

    // Executando Configuração 1 de Parâmetros
    executarSimulatedAnnealingTSP(cidades, matriz, 10000.0, 0.001, 1000, 0.99, 1);

    // Executando Configuração 2 de Parâmetros (Valores alterados para teste comparativo)
    executarSimulatedAnnealingTSP(cidades, matriz, 50000.0, 0.0001, 500, 0.95, 2);

    return 0;
}