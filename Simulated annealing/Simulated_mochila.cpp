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

// Estrutura para representar um item da mochila
struct Item {
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

// Função de avaliação para o Problema da Mochila 0/1
int avaliar_mochila(const vector<int>& s, const vector<Item>& itens, int capacidade, int& peso_total_out) {
    int peso_total = 0;
    int valor_total = 0;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == 1) {
            peso_total += itens[i].peso;
            valor_total += itens[i].valor;
        }
    }
    peso_total_out = peso_total;
    if (peso_total > capacidade) {
        return 0; // Solução inválida (excede a capacidade da mochila)
    }
    return valor_total;
}

// Estrutura para armazenar os resultados de cada execução
struct ResultadoExecucao {
    int execucao;
    int valor_obj;
    int peso_total;
    double tempo_ms;
};

void executarSimulatedAnnealingMochila(const vector<Item>& itens, int capacidade, double T_init, double T_min, int SAmax, double alpha, int config_id) {
    int n = itens.size();
    vector<ResultadoExecucao> resultados(10);
    double tempo_total = 0;

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> disReal(0.0, 1.0);
    uniform_int_distribution<> disInt(0, n - 1);

    auto inicio_geral = chrono::high_resolution_clock::now();

    for (int exec = 0; exec < 10; exec++) {
        auto inicio_exec = chrono::high_resolution_clock::now();

        // Solução inicial inteligente: sorteia itens aleatoriamente mas só coloca 
        // se o peso atual couber na mochila (evita começar em 0 e estagnar baixo)
        vector<int> s(n, 0);
        int peso_atual = vector<int>(n, 0).size(); // apenas referência
        int peso_temp = 0;
        
        // Embaralha os índices para pegar itens aleatórios
        vector<int> indices(n);
        for(int i = 0; i < n; i++) indices[i] = i;
        shuffle(indices.begin(), indices.end(), gen);

        for (int i = 0; i < n; i++) {
            int idx = indices[i];
            if (peso_temp + itens[idx].peso <= capacidade && disReal(gen) < 0.6) {
                s[idx] = 1;
                peso_temp += itens[idx].peso;
            }
        }

        int peso_s = 0;
        int valor_s = avaliar_mochila(s, itens, capacidade, peso_s);

        vector<int> s_best = s;
        int valor_best = valor_s;

        double T = T_init;
        int iteracoes_T = 0;

        while (T > T_min) {
            while (iteracoes_T < SAmax) {
                iteracoes_T++;

                // Vizinhança: Inverter um bit (adicionar ou remover um item)
                vector<int> s_vizinho = s;
                int pos = disInt(gen);
                s_vizinho[pos] = 1 - s_vizinho[pos]; 
                
                int peso_vizinho = 0;
                int valor_vizinho = avaliar_mochila(s_vizinho, itens, capacidade, peso_vizinho);

                // Como é um problema de MAXIMIZAÇÃO, queremos valor maior
                if (valor_vizinho > valor_s) {
                    s = s_vizinho;
                    valor_s = valor_vizinho;
                    
                    if (valor_vizinho > valor_best) {
                        s_best = s_vizinho;
                        valor_best = valor_vizinho;
                    }
                } 
                else {
                    // Critério de aceitação de piora (Metropolis para maximização)
                    double delta = valor_s - valor_vizinho; // positivo
                    double probabilidade = exp(-delta / T);
                    if (disReal(gen) < probabilidade) {  
                        s = s_vizinho;
                        valor_s = valor_vizinho;
                    }
                }
            }
            T = alpha * T;   // Resfriamento
            iteracoes_T = 0; 
        }

        auto fim_exec = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> duracao = fim_exec - inicio_exec;

        resultados[exec].execucao = exec + 1;
        resultados[exec].valor_obj = valor_best;
        int peso_best_final = 0;
        avaliar_mochila(s_best, itens, capacidade, peso_best_final);
        resultados[exec].peso_total = peso_best_final;
        resultados[exec].tempo_ms = duracao.count();
        tempo_total += resultados[exec].tempo_ms;
    }

    auto fim_geral = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duracao_total = fim_geral - inicio_geral;

    // Cálculo das estatísticas (Melhor, Pior, Média)
    int melhor = resultados[0].valor_obj;
    int pior = resultados[0].valor_obj;
    double soma_valores = 0;

    for (int i = 0; i < 10; i++) {
        if (resultados[i].valor_obj > melhor) melhor = resultados[i].valor_obj;
        if (resultados[i].valor_obj < pior) pior = resultados[i].valor_obj;
        soma_valores += resultados[i].valor_obj;
    }
    double media = soma_valores / 10.0;

    // Exibição da Tabela exigida pelo trabalho
    cout << "\n===================================================================\n";
    cout << " TABELA DE RESULTADOS - CONFIGURACAO " << config_id << "\n";
    cout << " (T0 = " << T_init << ", alpha = " << alpha << ", SAmax = " << SAmax << ")\n";
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

    if (dados.empty() || dados.size() < 2) {
        cout << "Erro: Arquivo vazio, não encontrado ou com formato inválido." << endl;
        return 1;
    }

    int numItens = dados[0];
    int capacidade = dados[1];
    
    vector<Item> itens;
    int index = 2;

    for (int i = 0; i < numItens && index < dados.size(); ++i) {
        Item item;
        item.id = i + 1;
        item.valor = dados[index];     
        item.peso = dados[index + 1];  
        itens.push_back(item);
        index += 2;
    }



    vector<int> dados2 = extrairDadosLimpos("mochila_100_1000_1");

    if (dados2.empty() || dados2.size() < 2) {
        cout << "Erro: Arquivo vazio, não encontrado ou com formato inválido." << endl;
        return 1;
    }

    int numItens2 = dados2[0];
    int capacidade2 = dados2[1];
    
    vector<Item> itens2;
    int index2 = 2;

    for (int i = 0; i < numItens2 && index2 < dados2.size(); ++i) {
        Item item2;
        item2.id = i + 1;
        item2.valor = dados2[index2];     
        item2.peso = dados2[index2 + 1];  
        itens2.push_back(item2);
        index2 += 2;
    }
    // Executando Configuração 1 de Parâmetros
    executarSimulatedAnnealingMochila(itens, capacidade, 10000.0, 0.001, 1000, 0.99, 1);

    // Executando Configuração 2 de Parâmetros
    executarSimulatedAnnealingMochila(itens, capacidade, 50000.0, 0.0001, 500, 0.95, 2);

        // Executando Configuração 1 de Parâmetros
    executarSimulatedAnnealingMochila(itens2, capacidade2, 10000.0, 0.001, 1000, 0.99, 1);

    // Executando Configuração 2 de Parâmetros
    executarSimulatedAnnealingMochila(itens2, capacidade2, 50000.0, 0.0001, 500, 0.95, 2);

    return 0;
}