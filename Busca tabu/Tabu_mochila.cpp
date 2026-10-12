#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>

using namespace std;

struct ItemMochila {
    int peso;
    int valor;
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

int avaliar_mochila_penalidade(const string& solucao, const vector<ItemMochila>& itens, int cap, int& peso_total_out) {
    int peso = 0, valor = 0;

    for (size_t i = 0; i < solucao.length(); i++) {
        if (solucao[i] == '1') {
            peso += itens[i].peso;
            valor += itens[i].valor;
        }
    }

    peso_total_out = peso;

    if (peso > cap) {
        int excesso = peso - cap;
        int penalidade = excesso * 10;
        int obj = valor - penalidade;
        return (obj < 0) ? 0 : obj;
    }

    return valor;
}

void executarTabuMochila(const vector<ItemMochila>& itens, int cap, int tenure_param, int max_sem_melhora_param, int max_iter_param, int config_id) {
    int n = itens.size();

    auto inicio = chrono::high_resolution_clock::now();

    string solucao_atual(n, '0');

    int peso_atual = 0;
    int valor_atual = avaliar_mochila_penalidade(solucao_atual, itens, cap, peso_atual);

    string melhor_solucao_global = solucao_atual;
    int melhor_valor_global = valor_atual;
    int melhor_peso_global = peso_atual;

    vector<int> lista_tabu(n, 0);
    int tenure = (tenure_param == -1) ? max(2, n / 4) : tenure_param;

    int iter_sem_melhora = 0;
    int iteracoes_totais = 0;

    for (int iter = 0; iter < max_iter_param && iter_sem_melhora < max_sem_melhora_param; iter++) {
        iteracoes_totais++;

        int melhor_movimento = -1;
        int melhor_valor_vizinho = -999999;

        int melhor_movimento_proibido = -1;
        int melhor_valor_proibido = -999999;

        for (int i = 0; i < n; i++) {
            string vizinho = solucao_atual;
            vizinho[i] = (vizinho[i] == '0') ? '1' : '0';

            int peso_viz = 0;
            int valor_viz = avaliar_mochila_penalidade(vizinho, itens, cap, peso_viz);

            bool e_tabu = (lista_tabu[i] > 0);
            bool aspira_objetivo = e_tabu && (valor_viz > melhor_valor_global);

            if (!e_tabu || aspira_objetivo) {
                if (valor_viz > melhor_valor_vizinho) {
                    melhor_valor_vizinho = valor_viz;
                    melhor_movimento = i;
                }
            } else {
                if (valor_viz > melhor_valor_proibido) {
                    melhor_valor_proibido = valor_viz;
                    melhor_movimento_proibido = i;
                }
            }
        }

        if (melhor_movimento == -1) {
            melhor_movimento = melhor_movimento_proibido;
            melhor_valor_vizinho = melhor_valor_proibido;
        }

        if (melhor_movimento == -1) break;

        solucao_atual[melhor_movimento] =
            (solucao_atual[melhor_movimento] == '0') ? '1' : '0';

        int dummy_peso = 0;
        valor_atual = avaliar_mochila_penalidade(
            solucao_atual, itens, cap, dummy_peso
        );

        for (int i = 0; i < n; i++) {
            if (lista_tabu[i] > 0) lista_tabu[i]--;
        }

        lista_tabu[melhor_movimento] = tenure;

        if (dummy_peso <= cap && valor_atual > melhor_valor_global) {
            melhor_valor_global = valor_atual;
            melhor_solucao_global = solucao_atual;
            melhor_peso_global = dummy_peso;
            iter_sem_melhora = 0;
        } else {
            iter_sem_melhora++;
        }
    }

    auto fim = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duracao = fim - inicio;

    cout << "\n===================================================================\n";
    cout << " TABELA DE RESULTADOS (MOCHILA - BUSCA TABU) - CONFIGURACAO " << config_id << "\n";
    cout << " (Tenure = " << tenure << ", Max Iter Sem Melhora = " << max_sem_melhora_param << ")\n";
    cout << "===================================================================\n";

    cout << left << setw(15) << "Valor (Obj)"
         << setw(15) << "Peso Total"
         << setw(15) << "Iteracoes"
         << setw(15) << "Tempo (ms)" << endl;

    cout << "-------------------------------------------------------------------\n";

    cout << left << setw(15) << melhor_valor_global
         << setw(15) << melhor_peso_global
         << setw(15) << iteracoes_totais
         << fixed << setprecision(2) << duracao.count() << endl;

    cout << "===================================================================\n";
}

int main() {
    vector<int> dados = extrairDadosLimpos("mochila.txt");

    if (dados.size() < 2) {
        cout << "Erro: arquivo vazio ou formato invalido.\n";
        return 1;
    }

    int n = dados[0];
    int cap = dados[1];

    if (n < 0 || cap < 0 || dados.size() != 2 + 2 * n) {
        cout << "Erro: formato invalido no arquivo mochila.txt.\n";
        return 1;
    }

    vector<ItemMochila> itens;

    int index = 2;

    for (int i = 0; i < n; i++) {
        int peso = dados[index];
        int valor = dados[index + 1];

        if (peso < 0) {
            cout << "Erro: peso invalido no arquivo mochila.txt.\n";
            return 1;
        }

        itens.push_back({peso, valor});
        index += 2;
    }

    executarTabuMochila(itens, cap, -1, 100, 1000, 1);

    executarTabuMochila(itens, cap, 5, 50, 500, 2);

    return 0;
}