/*-------------------------------------*/
/*--FITXER DE PROCES DE CADA FANTASMA--*/
/*-----------Enrique Molina------------*/
/*-------------------------------------*/

#include <pthread.h>
#include <stdio.h>	   		/* incloure definicions de funcions estandard */
#include <stdlib.h>		   	/* per exit() */
#include <unistd.h>		   	/* per getpid() */
#include "winsuport2.h"		/* incloure definicions de funcions propies */
#include "memoria.h"
#include "semafor.h"
#include "missatge.h"

typedef struct {		/* per un objecte (menjacocos o fantasma) */
	int f;							/* posicio actual: fila */
	int c;							/* posicio actual: columna */
	int d;							/* direccio actual: [0..3] */
	char a;							/* caracter anterior en pos. actual */
} objecte;

int df[] = {-1, 0, 1, 0};				/* moviments de les 4 direccions possibles */
int dc[] = {0, -1, 0, 1};				/* dalt, esquerra, baix, dreta */


int main(int n_args, char *ll_args[])
{
		/* VARIABLES */
		objecte f, seg;
    int n_fil, n_col;
    int camp_joc;
		int id_sem1, id_sem2, id_bus1, id_bus2;
  	int k, vk, nd, vd[3];

		srand(getpid());

		/* PREPARACIO DELS ARGUMENTS */
    camp_joc = atoi(ll_args[1]);
    n_fil = atoi(ll_args[2]);
    n_col = atoi(ll_args[3]);
    f.f = atoi(ll_args[4]);
    f.c = atoi(ll_args[5]);
    f.d = atoi(ll_args[6]);
		f.a='.';
		id_sem1=atoi(ll_args[7]);
		id_sem2=atoi(ll_args[8]);
		id_bus1=atoi(ll_args[9]);
		id_bus2=atoi(ll_args[10]);

		int choc = atoi(ll_args[11]);
		int retard = atoi(ll_args[12]);

		void* id_win = map_mem(camp_joc);

		void* id_choc = map_mem(choc);
		char *n_choc = (char *)id_choc;

		char aux[2];
		receiveM(id_bus1, aux);
		char index=aux[0];
	//	ind_bustia=n_choc[1]-1;

		char next;

  	/* BUCLE DE MOVIMENT */
		do
    {
        win_set(id_win, n_fil, n_col);
				nd = 0;

				/* ACTIVACIO DELS FANTASMES */
				next = win_quincar(f.f + df[f.d], f.c + dc[f.d]);
				if(next=='+' && n_choc[1]<n_choc[2])							/* comprobacio choque computable */
				{
						f.d = (f.d + 2) % 4;
						waitS(id_sem2);
						n_choc[0]++;
						if(n_choc[0]==2)
						{
							char aux[2];
							sprintf(aux, "%c", '1'+n_choc[1]);
							sendM(id_bus1, aux, 2);
							n_choc[0]=0;
							n_choc[1]++;
						}
						signalS(id_sem2);
				}


  		  for (k=-1; k<=1; k++)													/* provar direccio actual i dir. veines */
  		  {
			       vk = (f.d + k) % 4;											/* direccio veina */
			       if (vk < 0) vk += 4;											/* corregeix negatius */
			       seg.f = f.f + df[vk]; 										/* calcular posicio en la nova dir.*/
			       seg.c = f.c + dc[vk];
			       seg.a = win_quincar(seg.f,seg.c);				/* calcular caracter seguent posicio */
			       if ((seg.a==' ') || (seg.a=='.') || (seg.a=='0'))
			       {
					          vd[nd] = vk;											/* memoritza com a direccio possible */
			  	          nd++;
	           }
  		  }

				if (nd == 0)																	/* si no pot continuar, */
  			     f.d = (f.d + 2) % 4;													/* canvia totalment de sentit */
  		  else
  		  {
			      if (nd == 1)															/* si nomes pot en una direccio */
	  			         f.d = vd[0];														/* li assigna aquesta */
			      else																			/* altrament */
				           f.d = vd[rand() % nd];									/* segueix una dir. aleatoria */
      			seg.f = f.f + df[f.d];  									/* calcular seguent posicio final */
      			seg.c = f.c + dc[f.d];
      			seg.a = win_quincar(seg.f,seg.c);					/* calcular caracter seguent posicio */
waitS(id_sem1);
      			win_escricar(f.f,f.c,f.a,NO_INV);					/* esborra posicio anterior */
      			f.f = seg.f; f.c = seg.c; f.a = seg.a;		/* actualitza posicio */
      			win_escricar(f.f,f.c,index,NO_INV);				/* redibuixa fantasma */
signalS(id_sem1);
		       	if ((f.a == '0') || ('0'==win_quincar(f.f + df[f.d], f.c + dc[f.d])))								/* ha capturat menjacocos */
						{
							char aux[2];
							sprintf(aux, "%c", 1);
							sendM(id_bus2, aux, 2);
						}
		  	 	  }
		    win_retard(retard);
	   }while(1);
  return 0;
}
