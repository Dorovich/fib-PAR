#include "omp.h"

#define lowerb(id, p, n)  ( id * (n/p) + (id < (n%p) ? id : n%p) )
#define numElem(id, p, n) ( (n/p) + (id < (n%p)) )
#define upperb(id, p, n)  ( lowerb(id, p, n) + numElem(id, p, n) - 1 )

#define min(a, b) ( (a < b) ? a : b )
#define max(a, b) ( (a > b) ? a : b )

extern int userparam;

// Function to copy one matrix into another
void copy_mat (double *u, double *v, unsigned sizex, unsigned sizey) {

    int nblocksi=omp_get_max_threads();
    int nblocksj=20;

#pragma omp parallel
    {
        int blocki = omp_get_thread_num();
        int i_start = lowerb(blocki, nblocksi, sizex);
        int i_end = upperb(blocki, nblocksi, sizex);
        for (int blockj=0; blockj<nblocksj; ++blockj) {
            int j_start = lowerb(blockj, nblocksj, sizey);
            int j_end = upperb(blockj, nblocksj, sizey);
            for (int i=max(1, i_start); i<=min(sizex-2, i_end); i++)
                for (int j=max(1, j_start); j<=min(sizey-2, j_end); j++)
                    v[i*sizey+j] = u[i*sizey+j];
        }
    }
}

double solve_gauss (double *u, unsigned sizex, unsigned sizey);

// 2D-blocked solver: one iteration step
double solve (double *u, double *unew, unsigned sizex, unsigned sizey) {
    double tmp, diff, sum=0.0;

    if (u==unew) return solve_gauss(u,sizex,sizey);

    int nblocksi=omp_get_max_threads();
    int nblocksj=1;

#pragma omp parallel reduction(+: sum) private(tmp, diff)
    {
        int blocki = omp_get_thread_num();
        int i_start = lowerb(blocki, nblocksi, sizex);
        int i_end = upperb(blocki, nblocksi, sizex);
        for (int blockj=0; blockj<nblocksj; ++blockj) {
            int j_start = lowerb(blockj, nblocksj, sizey);
            int j_end = upperb(blockj, nblocksj, sizey);
            for (int i=max(1, i_start); i<=min(sizex-2, i_end); i++) {
                for (int j=max(1, j_start); j<=min(sizey-2, j_end); j++) {
                    tmp = 0.25 * ( u[ i*sizey	   + (j-1) ] +  // left
                                   u[ i*sizey	   + (j+1) ] +  // right
                                   u[ (i-1)*sizey + j     ] +  // top
                                   u[ (i+1)*sizey + j     ] ); // bottom
                    diff = tmp - u[i*sizey+ j];
                    sum += diff * diff;
                    unew[i*sizey+j] = tmp;
                }
            }
        }
    }

    return sum;
}

// 2D-blocked solver: one iteration step
double solve_gauss (double *u, unsigned sizex, unsigned sizey) {
    double tmp, diff, sum=0.0;

    int P=omp_get_max_threads();
    int nblocksi=P;
    int nblocksj=20;
    int next[P];

    for (int i=0; i<P; i++) next[i]=0;

#pragma omp parallel reduction(+: sum) private(tmp, diff) num_threads(P)
    {
        int myid = omp_get_thread_num();
        int tmp_next;

      	int blocki = omp_get_thread_num();
        int i_start = lowerb(blocki, nblocksi, sizex);
        int i_end = upperb(blocki, nblocksi, sizex);
        for (int blockj=0; blockj<nblocksj; ++blockj) {
            int j_start = lowerb(blockj, nblocksj, sizey);
            int j_end = upperb(blockj, nblocksj, sizey);
				
            if (myid > 0) {
                do {
#pragma omp atomic read
                    tmp_next = next[myid-1];
                } while (tmp_next<=next[myid]);
            }

            for (int i=max(1, i_start); i<=min(sizex-2, i_end); i++) {
                for (int j=max(1, j_start); j<=min(sizey-2, j_end); j++) {
                    tmp = 0.25 * (	u[ i*sizey	   + (j-1) ] +  // left
                                        u[ i*sizey	   + (j+1) ] +  // right
                                        u[ (i-1)*sizey + j     ] +  // top
                                        u[ (i+1)*sizey + j     ] ); // bottom
                    diff = tmp - u[i*sizey+ j];
                    sum += diff * diff;
                    u[i*sizey+j] = tmp;
                }
            }
#pragma omp atomic update
            next[myid]++;
        }
    }
    return sum;
}
