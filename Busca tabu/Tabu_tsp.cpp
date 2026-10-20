
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iomanip>

using namespace std;

struct Cidade {
    int id;
    int x;
    int y;
};

// Funcao reaproveitada para limpeza e leitura do arquivo
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
    int custo = 0;
    int n = caminho.size();

    for (int i = 0; i < n - 1; i++) {
        custo += matriz[caminho[i]][caminho[i + 1]];
    }

    custo += matriz[caminho[n - 1]][caminho[0]];

    return custo;
}

void executarTabuTSP(
    const vector<Cidade>& cidades,
    const vector<vector<int>>& matriz,
    int tenure_param,
    int max_sem_melhora_param,
    int max_iter_param,
    int config_id
) {
    int n = cidades.size();

    if (n < 2) {
        cout << "Erro: o TSP precisa de pelo menos duas cidades." << endl;
        return;
    }

    auto inicio = chrono::high_resolution_clock::now();

    // Solucao inicial: cidades na ordem de leitura
    vector<int> caminho(n);

    for (int i = 0; i < n; i++) {
        caminho[i] = i;
    }

    int custo_atual = avaliar_tsp(caminho, matriz);

    vector<int> melhor_caminho = caminho;
    int melhor_custo = custo_atual;

    // Configuracao da duracao tabu
    int tenure;

    if (tenure_param == -1) {
        tenure = max(2, n / 4);
    } else {
        tenure = tenure_param;
    }

    // Guarda ate quando cada troca fica proibida
    vector<vector<int>> lista_tabu(
        n, vector<int>(n, 0)
    );

    int iteracao = 0;
    int sem_melhora = 0;

    while (iteracao < max_iter_param &&
           sem_melhora < max_sem_melhora_param) {

        iteracao++;

        vector<int> melhor_vizinho;
        int melhor_custo_vizinho = 2147483647;
        int pos1_escolhida = -1;
        int pos2_escolhida = -1;

        // Testa todas as trocas entre duas cidades
        for (int i = 0; i < n - 1; i++) {
            for (int j = i + 1; j < n; j++) {

                // Ignora trocas que ainda estao tabu
                if (lista_tabu[i][j] > iteracao) {
                    continue;
                }

                vector<int> vizinho = caminho;
                swap(vizinho[i], vizinho[j]);

                int custo_vizinho = avaliar_tsp(vizinho, matriz);

                if (custo_vizinho < melhor_custo_vizinho) {
                    melhor_custo_vizinho = custo_vizinho;
                    melhor_vizinho = vizinho;
                    pos1_escolhida = i;
                    pos2_escolhida = j;
                }
            }
        }

        // Se nao houver vizinho permitido, passa para a proxima iteracao
        if (pos1_escolhida == -1) {
            sem_melhora++;
            continue;
        }

        // Aceita o melhor vizinho permitido
        caminho = melhor_vizinho;
        custo_atual = melhor_custo_vizinho;

        // A troca realizada fica tabu durante o tenure definido
        lista_tabu[pos1_escolhida][pos2_escolhida] =
            iteracao + tenure;

        // Atualiza a melhor solucao global
        if (custo_atual < melhor_custo) {
            melhor_custo = custo_atual;
            melhor_caminho = caminho;
            sem_melhora = 0;
        } else {
            sem_melhora++;
        }
    }

    auto fim = chrono::high_resolution_clock::now();

    chrono::duration<double, milli> duracao = fim - inicio;

    cout << "\n============================================================\n";
    cout << " RESULTADOS TSP - BUSCA TABU - CONFIGURACAO "
         << config_id << "\n";
    cout << "============================================================\n";

    cout << "Duracao tabu: " << tenure << " iteracoes" << endl;
    cout << "Maximo de iteracoes: " << max_iter_param << endl;
    cout << "Maximo sem melhora: " << max_sem_melhora_param << endl;
    cout << "Melhor custo encontrado: " << melhor_custo << endl;
    cout << "Iteracoes executadas: " << iteracao << endl;
    cout << "Tempo de execucao: " << fixed << setprecision(2)
         << duracao.count() << " ms" << endl;

    cout << "Melhor rota: ";

    for (int i = 0; i < n; i++) {
        cout << cidades[melhor_caminho[i]].id;

        if (i < n - 1) cout << " -> ";
    }

    cout << " -> " << cidades[melhor_caminho[0]].id << endl;
    cout << "============================================================\n";
}

int main() {
    vector<int> dados = extrairDadosLimpos("tsp_5");

    if (dados.empty() || dados.size() % 3 != 0) {
        cout << "Erro: arquivo vazio, nao encontrado ou com formato "
             << "invalido (esperado: id x y)." << endl;
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

    vector<vector<int>> matriz(
        numVertices, vector<int>(numVertices, 0)
    );

    for (int i = 0; i < numVertices; ++i) {
        for (int j = 0; j < numVertices; ++j) {
            if (i != j) {
                double dx = cidades[i].x - cidades[j].x;
                double dy = cidades[i].y - cidades[j].y;

                matriz[i][j] = round(sqrt(dx * dx + dy * dy));
            }
        }
    }

    // Configuracao 1
    executarTabuTSP(cidades, matriz, -1, 100, 1000, 1);

    // Configuracao 2
    executarTabuTSP(cidades, matriz, 5, 50, 500, 2);


    vector<int> dados2 = extrairDadosLimpos("tsp_51");

    if (dados2.empty() || dados2.size() % 3 != 0) {
        cout << "Erro: arquivo vazio, nao encontrado ou com formato "
             << "invalido (esperado: id x y)." << endl;
        return 1;
    }

    int numVertices2 = dados2.size() / 3;
    vector<Cidade> cidades2(numVertices2);

    int index2 = 0;

    for (int i = 0; i < numVertices2; ++i) {
        cidades2[i].id = dados2[index2];
        cidades2[i].x = dados2[index2 + 1];
        cidades2[i].y = dados2[index2 + 2];
        index2 += 3;
    }

    vector<vector<int>> matriz2(
        numVertices2, vector<int>(numVertices2, 0)
    );

    for (int i = 0; i < numVertices2; ++i) {
        for (int j = 0; j < numVertices2; ++j) {
            if (i != j) {
                double dx2 = cidades2[i].x - cidades2[j].x;
                double dy2 = cidades2[i].y - cidades2[j].y;

                matriz2[i][j] = round(sqrt(dx2 * dx2 + dy2 * dy2));
            }
        }
    }

    // Configuracao 1
    executarTabuTSP(cidades2, matriz2, -1, 100, 1000, 1);

    // Configuracao 2
    executarTabuTSP(cidades2, matriz2, 5, 50, 500, 2);

    return 0;
}
