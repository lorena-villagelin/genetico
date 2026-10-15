#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>
#include <chrono>
using namespace std;

struct Ponto { double x, y; };
struct Individuo { vector<int> rota; double custo; };

int N;
vector<Ponto> pts;
vector<vector<double>> D;
mt19937 rng(42);

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
    for (int i = 1; i < 3; i++) {
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

int main(int argc, char** argv) {
    int cenario = argc > 1 ? atoi(argv[1]) : 0;
    N           = argc > 2 ? atoi(argv[2]) : 20;
    int POP     = argc > 3 ? atoi(argv[3]) : 200;
    int MAXGEN  = argc > 4 ? atoi(argv[4]) : 500;
    double MUT  = argc > 5 ? atof(argv[5]) : 0.2;
    int PARADA  = argc > 6 ? atoi(argv[6]) : 100;
    int PRINT   = argc > 7 ? atoi(argv[7]) : 1;
    const double CRUZ = 0.9;
    const int ELITE = 2;
    if (N < 8) N = 8;

    const double PI = acos(-1.0);
    for (int i = 0; i < N; i++) {
        if (cenario == 0) pts.push_back({rnd01() * 100, rnd01() * 100});
        else pts.push_back({50 + 40 * cos(2 * PI * i / N), 50 + 40 * sin(2 * PI * i / N)});
    }
    if (cenario == 1) shuffle(pts.begin(), pts.end(), rng);
    D.assign(N, vector<double>(N));
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            D[i][j] = hypot(pts[i].x - pts[j].x, pts[i].y - pts[j].y);

    if (cenario == 1)
        printf("Otimo teorico (circulo): %.4f\n", 2 * N * 40 * sin(PI / N));

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

    FILE* f = fopen("melhor_rota.csv", "w");
    fprintf(f, "x,y\n");
    for (int c : pop[0].rota) fprintf(f, "%f,%f\n", pts[c].x, pts[c].y);
    fprintf(f, "%f,%f\n", pts[pop[0].rota[0]].x, pts[pop[0].rota[0]].y);
    fclose(f);
    return 0;
}
