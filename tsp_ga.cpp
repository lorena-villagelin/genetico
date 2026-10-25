#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>
#include <chrono>
using namespace std;

// parametros
const int    CENARIO   = 0;     // 0 = pontos aleatorios, 1 = circulo
const int    N         = 20;    // quantidade de pontos (minimo 8)
const int    POP       = 200;   // tamanho da populacao
const int    MAXGEN    = 500;   // maximo de geracoes
const double MUT       = 0.2;   // taxa de mutacao (0 a 1)
const double CRUZ      = 0.9;   // taxa de cruzamento (0 a 1)
const int    ELITE     = 2;     // quantos melhores passam direto
const int    K_TORNEIO = 3;     // quantos competem no torneio
const int    PARADA    = 100;   // para se ficar X geracoes sem melhora
const int    PRINT     = 1;     // imprime a cada X geracoes
const unsigned int SEED = 42;   // semente do gerador aleatorio
const double LADO      = 100;   // lado da area dos pontos aleatorios
const double RAIO      = 40;    // raio do circulo

struct Ponto { double x, y; };
struct Individuo { vector<int> rota; double custo; };

vector<Ponto> pts;
vector<vector<double>> D;
mt19937 rng(SEED);

int rnd(int n) { return rng() % n; }
double rnd01() { return uniform_real_distribution<double>(0, 1)(rng); }

double comprimento(const vector<int>& r) {
    double s = 0;
    for (int i = 0; i < N; i++) s += D[r[i]][r[(i + 1) % N]];
    return s;
}
double aptidao(const Individuo& ind) { return 1.0 / ind.custo; }

const Individuo& torneio(const vector<Individuo>& pop) {
    const Individuo* melhor = &pop[rnd(pop.size())];
    for (int i = 1; i < K_TORNEIO; i++) {
        const Individuo& c = pop[rnd(pop.size())];
        if (aptidao(c) > aptidao(*melhor)) melhor = &c;
    }
    return *melhor;
}

vector<int> cruzamentoOX(const vector<int>& p1, const vector<int>& p2) {
    int a = rnd(N), b = rnd(N);
    if (a > b) swap(a, b);
    vector<int> filho(N, -1);
    vector<bool> usado(N, false);
    for (int i = a; i <= b; i++) { filho[i] = p1[i]; usado[p1[i]] = true; }
    int pos = (b + 1) % N;
    for (int k = 0; k < N; k++) {
        int cidade = p2[(b + 1 + k) % N];
        if (!usado[cidade]) { filho[pos] = cidade; pos = (pos + 1) % N; }
    }
    return filho;
}

void mutacao(vector<int>& r) {
    int a = rnd(N), b = rnd(N);
    if (a > b) swap(a, b);
    reverse(r.begin() + a, r.begin() + b + 1);
}

int main() {
    printf("=========== PARAMETROS USADOS ===========\n");
    printf("Cenario ................ %d (%s)\n", CENARIO, CENARIO == 0 ? "pontos aleatorios" : "circulo");
    printf("Quantidade de pontos ... %d\n", N);
    printf("Populacao .............. %d\n", POP);
    printf("Maximo de geracoes ..... %d\n", MAXGEN);
    printf("Taxa de mutacao ........ %.2f\n", MUT);
    printf("Taxa de cruzamento ..... %.2f\n", CRUZ);
    printf("Elite .................. %d\n", ELITE);
    printf("Tamanho do torneio ..... %d\n", K_TORNEIO);
    printf("Parada (sem melhora) ... %d geracoes\n", PARADA);
    printf("Imprime a cada ......... %d geracoes\n", PRINT);
    printf("Seed ................... %u\n", SEED);
    if (CENARIO == 0) printf("Area dos pontos ........ %.0f x %.0f\n", LADO, LADO);
    else              printf("Raio do circulo ........ %.0f\n", RAIO);
    printf("=========================================\n\n");

    const double PI = acos(-1.0);
    for (int i = 0; i < N; i++) {
        if (CENARIO == 0) pts.push_back({rnd01() * LADO, rnd01() * LADO});
        else pts.push_back({RAIO + RAIO * cos(2 * PI * i / N), RAIO + RAIO * sin(2 * PI * i / N)});
    }
    if (CENARIO == 1) shuffle(pts.begin(), pts.end(), rng);
    D.assign(N, vector<double>(N));
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            D[i][j] = hypot(pts[i].x - pts[j].x, pts[i].y - pts[j].y);

    if (CENARIO == 1)
        printf("Otimo teorico (circulo): %.4f\n\n", 2 * N * RAIO * sin(PI / N));

    vector<Individuo> pop(POP);
    for (auto& ind : pop) {
        ind.rota.resize(N);
        iota(ind.rota.begin(), ind.rota.end(), 0);
        shuffle(ind.rota.begin(), ind.rota.end(), rng);
        ind.custo = comprimento(ind.rota);
    }

    auto cmp = [](const Individuo& a, const Individuo& b) { return a.custo < b.custo; };
    auto t0 = chrono::steady_clock::now();
    double melhorGlobal = 1e18;
    int semMelhora = 0, gen;

    for (gen = 0; gen < MAXGEN; gen++) {
        sort(pop.begin(), pop.end(), cmp);
        double media = 0;
        for (auto& i : pop) media += i.custo;
        media /= POP;

        if (pop[0].custo < melhorGlobal - 1e-9) { melhorGlobal = pop[0].custo; semMelhora = 0; }
        else semMelhora++;

        if (gen % PRINT == 0) {
            printf("Gen %5d | melhor %9.4f | media %9.4f | rota:", gen, pop[0].custo, media);
            if (N <= 50) for (int c : pop[0].rota) printf(" %d", c);
            printf("\n");
        }
        if (semMelhora >= PARADA) break;

        vector<Individuo> nova(pop.begin(), pop.begin() + ELITE);
        while ((int)nova.size() < POP) {
            const Individuo& p1 = torneio(pop);
            const Individuo& p2 = torneio(pop);
            Individuo filho;
            filho.rota = (rnd01() < CRUZ) ? cruzamentoOX(p1.rota, p2.rota) : p1.rota;
            if (rnd01() < MUT) mutacao(filho.rota);
            filho.custo = comprimento(filho.rota);
            nova.push_back(filho);
        }
        pop = nova;
    }

    sort(pop.begin(), pop.end(), cmp);
    double seg = chrono::duration<double>(chrono::steady_clock::now() - t0).count();
    printf("\nFIM na geracao %d | melhor distancia: %.4f | tempo: %.3f s\n", gen, pop[0].custo, seg);
    printf("Melhor rota:");
    if (N <= 50) for (int c : pop[0].rota) printf(" %d", c);
    printf("\n");
    return 0;
}
