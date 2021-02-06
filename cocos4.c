/*****************************************/
/*									     */
/*		          cocos1.c			 	 */
/*									     */
/*****************************************/


#include <pthread.h>
#include <stdio.h>			/* incloure definicions de funcions estandard */
#include <stdlib.h>			/* per exit() */
#include <unistd.h>			/* per getpid() */
#include "winsuport2.h"		/* incloure definicions de funcions propies */
#include "memoria.h"
#include "semafor.h"
#include "missatge.h"
#include <signal.h>
#include <string.h>



#define MIN_FIL 7			/* definir limits de variables globals */
#define MAX_FIL 25
#define MIN_COL 10
#define MAX_COL 80

#define MAX_FAN 10
#define MAX_THREADS 10
#define MAX_PROCESSOS 10

/* definir estructures d'informacio */
typedef struct {		/* per un objecte (menjacocos o fantasma) */
	int f;					/* posicio actual: fila */
	int c;					/* posicio actual: columna */
	int d;					/* direccio actual: [0..3] */
	char a;					/* caracter anterior en pos. actual */
} objecte;

int estat;

/* variables globals */
int n_fil1, n_col;		/* dimensions del camp de joc */
char tauler[70];			/* nom del fitxer amb el laberint de joc */
char c_req;						/* caracter de pared del laberint */

objecte mc;      				/* informacio del menjacocos */
objecte f;							/* informacio de sortida del fantasma(10 max) */

int df[] = {-1, 0, 1, 0};	/* moviments de les 4 direccions possibles */
int dc[] = {0, -1, 0, 1};	/* dalt, esquerra, baix, dreta */

int cocos;			/* numero restant de cocos per menjar */
int retard;			/* valor del retard de moviment, en mil.lisegons */

pthread_t tid[MAX_THREADS];
pid_t tpid[MAX_PROCESSOS];
int num_f;
char * n_choc;
int * pos_mc;

int id_sem1, id_sem2;
int id_bus1, id_bus2;

char str_cocos[12], str_time[10];

int * bus_fan;

/* funcio per realitzar la carrega dels parametres de joc emmagatzemats */
/* dins d'un fitxer de text, el nom del qual es passa per referencia a  */
/* 'nom_fit'; si es detecta algun problema, la funcio avorta l'execucio */
/* enviant un missatge per la sortida d'error i retornant el codi per-	*/
/* tinent al SO (segons comentaris al principi del programa).			*/
void carrega_parametres(const char *nom_fit)
{
  FILE *fit;

  fit = fopen(nom_fit,"rt");		/* intenta obrir fitxer */
  if (fit == NULL)
  {
		fprintf(stderr,"No s'ha pogut obrir el fitxer \'%s\'\n",nom_fit);
  	exit(2);
  }

  if (!feof(fit)) fscanf(fit,"%d %d %s %c\n",&n_fil1,&n_col,tauler,&c_req);
  else {
	fprintf(stderr,"Falten parametres al fitxer \'%s\'\n",nom_fit);
	fclose(fit);
	exit(2);
	}
  if ((n_fil1 < MIN_FIL) || (n_fil1 > MAX_FIL) ||
	(n_col < MIN_COL) || (n_col > MAX_COL))
  {
	fprintf(stderr,"Error: dimensions del camp de joc incorrectes:\n");
	fprintf(stderr,"\t%d =< n_fil1 (%d) =< %d\n",MIN_FIL,n_fil1,MAX_FIL);
	fprintf(stderr,"\t%d =< n_col (%d) =< %d\n",MIN_COL,n_col,MAX_COL);
	fclose(fit);
	exit(3);
  }

  if (!feof(fit)) fscanf(fit,"%d %d %d\n",&mc.f,&mc.c,&mc.d);
  else {
	fprintf(stderr,"Falten parametres al fitxer \'%s\'\n",nom_fit);
	fclose(fit);
	exit(2);
	}
  if ((mc.f < 1) || (mc.f > n_fil1-3) ||
	(mc.c < 1) || (mc.c > n_col-2) ||
	(mc.d < 0) || (mc.d > 3))
  {
	fprintf(stderr,"Error: parametres menjacocos incorrectes:\n");
	fprintf(stderr,"\t1 =< mc.f (%d) =< n_fil1-3 (%d)\n",mc.f,(n_fil1-3));
	fprintf(stderr,"\t1 =< mc.c (%d) =< n_col-2 (%d)\n",mc.c,(n_col-2));
	fprintf(stderr,"\t0 =< mc.d (%d) =< 3\n",mc.d);
	fclose(fit);
	exit(4);
  }

  if (!feof(fit)) fscanf(fit,"%d %d %d\n",&f.f,&f.c,&f.d);
  else {
	fprintf(stderr,"Falten parametres al fitxer \'%s\'\n",nom_fit);
	fclose(fit);
	exit(2);
	}
  if ((f.f < 1) || (f.f > n_fil1-3) ||
	(f.c < 1) || (f.c > n_col-2) ||
	(f.d < 0) || (f.d > 3))
    {
	fprintf(stderr,"Error: parametres fantasma 1 incorrectes:\n");
	fprintf(stderr,"\t1 =< f1.f (%d) =< n_fil1-3 (%d)\n",f.f,(n_fil1-3));
	fprintf(stderr,"\t1 =< f1.c (%d) =< n_col-2 (%d)\n",f.c,(n_col-2));
	fprintf(stderr,"\t0 =< f1.d (%d) =< 3\n",f.d);
	fclose(fit);
	exit(5);
    }
  fclose(fit);			/* fitxer carregat: tot OK! */
  printf("Joc del MenjaCocos\n\tTecles: \'%c\', \'%c\', \'%c\', \'%c\', RETURN-> sortir\n",
		TEC_AMUNT, TEC_AVALL, TEC_DRETA, TEC_ESQUER);
  printf("prem una tecla per continuar:\n");
  getchar();
}

/* funcio per inicialitar les variables i visualitzar l'estat inicial del joc */
void inicialitza_joc(void)
{
  int r,i,j;
  char strin[12];
  r = win_carregatauler(tauler,n_fil1-1,n_col,c_req);
  if (r == 0)
  {
    mc.a = win_quincar(mc.f,mc.c);
    if (mc.a == c_req) r = -6;					/* error: menjacocos sobre pared */
    else
    {
     f.a = win_quincar(f.f,f.c);
       if (f.a == c_req) r = -7;				/* error: fantasma sobre pared */
       else
       {
			cocos = 0;							/* compta el numero total de cocos */
			for (i=0; i<n_fil1-1; i++)
	  			for (j=0; j<n_col; j++)
	   				 if (win_quincar(i,j)=='.') cocos++;

      win_escricar(mc.f,mc.c,'0',NO_INV);
			win_escricar(f.f,f.c,'1',NO_INV);
        if (mc.a == '.') cocos--;				/* menja primer coco */
		sprintf(strin,"Cocos: %d", cocos); win_escristr(strin);
       }
    }
  }
  if (r != 0)
  {
	win_fi();
	fprintf(stderr,"Error: no s'ha pogut inicialitzar el joc:\n");
	switch (r)
	{
			case -1: fprintf(stderr,"  nom de fitxer erroni\n"); break;
	 		case -2: fprintf(stderr,"  numero de columnes d'alguna fila no coincideix amb l'amplada del tauler de joc\n"); break;
	  	case -3: fprintf(stderr,"  numero de columnes del laberint incorrecte\n"); break;
	 		case -4: fprintf(stderr,"  numero de files del laberint incorrecte\n"); break;
	  	case -5: fprintf(stderr,"  finestra de camp de joc no oberta\n"); break;
	  	case -6: fprintf(stderr,"  posicio inicial del menjacocos damunt la pared del laberint\n"); break;
	  	case -7: fprintf(stderr,"  posicio inicial del fantasma damunt la pared del laberint\n"); break;
	}
	exit(7);
  }
}

/* funcio per moure el menjacocos una posicio, en funcio de la direccio de   */
/* moviment actual; retorna -1 si s'ha premut RETURN, 1 si s'ha menjat tots  */
/* els cocos, i 0 altrament */
void *mou_menjacocos(void *nulo)
{
  	objecte seg;
  	int tec;
		int mov=1;

	do{
  		tec = win_gettec();
  		if (tec != 0)
   			switch (tec)		/* modificar direccio menjacocos segons tecla */
   			{
						case TEC_AMUNT:		mc.d = 0; break;
						case TEC_ESQUER:	mc.d = 1; break;
						case TEC_AVALL:		mc.d = 2; break;
						case TEC_DRETA:		mc.d = 3; break;
						case TEC_RETURN:											/* sortida del joc */
						{
								char aux[2];
								sprintf(aux, "%c", 0);
								sendM(id_bus2, aux, 2);
								return (void *)0;
		   			}
				}
		seg.f = mc.f + df[mc.d];	/* calcular seguent posicio */
		seg.c = mc.c + dc[mc.d];
		seg.a = win_quincar(seg.f,seg.c);	/* calcular caracter seguent posicio */
		if (seg.a!='+')
		{
			waitS(id_sem1);
			win_escricar(mc.f,mc.c,' ',NO_INV);		/* esborra posicio anterior */
			mc.f = seg.f; mc.c = seg.c;				/* actualitza posicio */
			win_escricar(mc.f,mc.c,'0',NO_INV);		/* redibuixa menjacocos */
			signalS(id_sem1);
			mov=1;
			if (seg.a == '.')
			{
				cocos--;
				sprintf(str_cocos,"C: %d", cocos);
				if (cocos==0)
				{
					char aux[2];
					sprintf(aux, "%c", 2);
					sendM(id_bus2, aux, 2);
					return (void *)0;
				}
			}
  	}
		else if (n_choc[1]<num_f)
		{
				waitS(id_sem2);
				if(mov)n_choc[0]++;
				mov=0;
		}
		if(n_choc[0]==2)
		{
			char aux[2];
			sprintf(aux, "%c", '1'+n_choc[1]);
			sendM(id_bus1, aux, 2);
			n_choc[0]=0;
			n_choc[1]++;
		}
		signalS(id_sem2);
		win_update();
		win_retard(retard);    // afegeix retard mov. menjacocos
	}while(1);
	return (void *)0;
}

/***************************
 Cronometre encarregat de
control.lar el temps de joc
****************************/
void* cronometre(void* nulo)
{
	int min=0,seg=0,hora=0;

	for(;;)												/* bucle de control */
	{
		seg++;
		if(seg==60){ min++; seg=0; }
		if(min==60){ hora++; min=0; }
		sprintf(str_time,"%i:%i:%i", hora, min, seg);
		sleep(1);
	}
}


/*********************************
Funcio encarregada de actualitzar
      l'informacio de joc
**********************************/
void* actualitza_info(void* nulo)
{
	char info[50];
	while(1)
	{
			info[0]='\0';									/* reseteo de cadena */
			strcat(info, str_time);
			strcat(info, " - ");				/* concatenacio de cadenes */
			strcat(info, str_cocos);
			win_escristr(info);						/* escriptura per pantalla */
			win_retard(retard);
	}
}


/* programa principal */
int main(int n_args, const char *ll_args[])
{
  	int rc;											/* variables locals */
  	srand(getpid());								/* inicialitza numeros aleatoris */

	int camp_joc;							
	int choc;


		/* COMPROBACIO DEL NOMBRE D'ARGUMENTS */
  	if ((n_args != 3) && (n_args !=4))
  	{
			fprintf(stderr,"Comanda: cocos0 fit_param num_fantasmes [retard]\n");
  		exit(1);
 		}

		/* CARREGA DE PARAMETRES */
  	carrega_parametres(ll_args[1]);

  	if (n_args == 4) retard = atoi(ll_args[3]);
  	else retard = 100;

  	rc = win_ini(&n_fil1,&n_col,'+',INVERS);	/* intenta crear taulell */
  	if (rc != 0)								/* si aconsegueix accedir a l'entorn CURSES */
  	{
			/* INICIALITZA CAMP DE JOC */
										/* zona de memoria compartida */
			camp_joc = ini_mem(rc);
			void* id_win = map_mem(camp_joc);
			set_filcol(id_win,n_fil1, n_col);
			inicialitza_joc();
			win_update();
			win_set(id_win, n_fil1, n_col);

			num_f = atoi(ll_args[2]);
			if(num_f < 0 || num_f > 10)	num_f = 3; 


			id_sem1 = ini_sem(1);
			id_sem2 = ini_sem(1);
			id_bus1 = ini_mis();
			id_bus2 = ini_mis();

			/* CHOQUES */
			choc = ini_mem(3);
			void* id_choc = map_mem(choc);
			n_choc = (char *)id_choc;
			n_choc[0] = 0;
			n_choc[1] = 0;
			n_choc[2] = num_f;
			/*CREACIO DEL MENJACOCOS*/
			pthread_create(&tid[0], NULL, mou_menjacocos, NULL);

			/*CREACIO DEL CRONOMETRE*/
			pthread_create(&tid[1],NULL,cronometre,NULL);

			/* CREACIO DEL MISSATGE */
			pthread_create(&tid[2],NULL,actualitza_info,NULL);

			/* PREPARACIO ARGUMENTS EXECLP */
			char cn_fil[5], cn_col[5], fx[5], cx[5], dx[5], ccj[25], cc[20], sem1[20], sem2[20], bus1[25], bus2[25], rt[20];
			sprintf(rt, "%i", retard);
			sprintf(cc, "%i", choc);
			sprintf(cx, "%i", f.c);
			sprintf(fx, "%i", f.f);
			sprintf(dx, "%i", f.d);
			sprintf(cn_col, "%i", n_col);
			sprintf(cn_fil, "%i", n_fil1);
			sprintf(ccj, "%i", camp_joc);
			sprintf(sem1, "%i", id_sem1);
			sprintf(sem2, "%i", id_sem2);
			sprintf(bus1, "%i", id_bus1);
			sprintf(bus2, "%i", id_bus2);

			/*CREACIO DELS FANTASMES*/
			int i=0, n=0;
			for(i=0; i<num_f; i++)
			{
				tpid[n]=fork();
				if(tpid[n]==(pid_t)0)
				{
					execlp("./fantasma4", "fantasma4", ccj, cn_fil, cn_col, fx, cx, dx, sem1, sem2, bus1, bus2, cc, rt, (char *)0);
					exit(0);
				}
				else if (tpid[n] > 0) n++;
			}

			char codi[2];
			receiveM(id_bus2, codi);		/* ESPERA A LA CONDICIO DE FI */

			int k=0;
			for(k=0; k<n; k++)
			{
				kill(tpid[k], SIGTERM);
			}

			win_fi();
			
			system("clear");

			switch(codi[0])							/* CONDICIO DE FI DEL JOC */
			{
				case 0 : printf("S'ha aturat el joc amb tecla RETURN!\n"); break;
				case 1 : printf("Ha guanyat l'ordinador!\n"); break;
				case 2 : printf("Ha guanyat l'usuari!\n"); break;
			}
  	}
  	else													/* CONDICIO D'ERROR */
  	{
			fprintf(stderr,"Error: no s'ha pogut crear el taulell:\n");
			switch (rc)
			{
				case -1: fprintf(stderr,"camp de joc ja creat!\n");
			  	break;
		  	case -2: fprintf(stderr,"no s'ha pogut inicialitzar l'entorn de curses!\n");
			  	break;
		 		case -3: fprintf(stderr,"les mides del camp demanades son massa grans!\n");
			  	break;
		  	case -4: fprintf(stderr,"no s'ha pogut crear la finestra!\n");
			  	break;
				}
			exit(6);
  	}
	elim_mem(camp_joc);
	elim_mem(choc);
	elim_sem(id_sem1);
	elim_sem(id_sem2);
	elim_mis(id_bus1);
	elim_mis(id_bus2);
  return(0);
}
