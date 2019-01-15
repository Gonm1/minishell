#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SIZE 1000
#define COMANDOS 100
char *username, homeDir[1000];

/* SE OBTIENE EL NOMBRE DE USUARIO */
void IniciarShell(){
	system("clear");
    username = getenv("USER");
    getcwd(homeDir, sizeof(homeDir));
}

/* SE OBTIENE LA DIRECCIÓN EN LA QUE SE ENCUENTRA
 Y SE IMPRIME POR PANTALLA */ 
void Prompt(){
	char dir[1000];
    getcwd(dir, sizeof(dir));
    printf("%s@Servalum:%s$: ",username ,dir);
}

/* SE RECIBE LA CADENA INGRESA POR EL USUARIO, SE 
	GUARDA EN LECTURACONSOLA Y SE ELIMINA EL "\n" */
int RecibirString(char* lecturaConsola){
    fgets(lecturaConsola, SIZE, stdin);
    if (strlen(lecturaConsola) != 1){
       	strtok(lecturaConsola, "\n");
    	return 0;
    }
    return 1;
}

/* SE  PROCESA LA CADENA EN BUSQUEDA DE UNA TUBERIA, SI SE ENCUENTRA 
	SEPARA EN DOS LA CADENA.RETORNA EL RESULTADO DE LA BUSQUEDA*/
int ProcesarTuberia(char *lecturaConsola, char **stringTuberia){
    int i = 0;
    while(i < 2){
        stringTuberia[i] = strsep(&lecturaConsola, "|");
        i++;
    }

    if (stringTuberia[1] == NULL)
        return 0; // NO SE ENCONTRO TUBERIA
    else
        return 1; // SE ENCONTRO TUBERIA
}

/* SE ENCARGA DE PROCESAR LA CADENA INGRESA SEPARANDO EN FUNCION DEL
 CARACTER " " Y GUARDANDO CADA ARGUMENTO OBTENIDO EN EL VECTOR ARGUMENTOS*/
void ProcesarEspacios(char *lecturaConsola, char **argumentos){
    int i = 0;
    while(i < COMANDOS){
        argumentos[i] = strsep(&lecturaConsola, " ");
        if(argumentos[i] == NULL)
            break;
        if(strlen(argumentos[i]) == 0)
            i--;
        i++;
    }
}

/* SE ENCARGA DE ABRIR UN ARCHIVO CON EL NOMBRE QUE SE INGRESO EN
 EL COMANDO, LUEGO REEMPLAZA EL STDIN POR EL ARCHIVO ABIERTO */
void RedirigirEntrada(char cadena[SIZE]){
    char *cadenaPtr;
    int fd;
    cadenaPtr = cadena;
    fd = open(cadena, O_RDONLY); //ABRE EL ARCHIVO EN SOLO LECTURA
    close(0); // CIERRA STDIN
    dup(fd); // ASIGNA EL ARCHIVO COMO ENTRADA
}

/* SE ENCARGA DE CERRAR STDOUT Y ABRIR O CREAR UN ARCHIVO CON EL NOMBRE INGRESADO 
  EN EL COMANDO, DE MANERA QUE REEMPLACE EL DESCRIPTOR DE ARCHIVO PREVIAMENTE CERRADO */
void RedirigirSalida(char cadena[SIZE]){
    char *cadenaPtr;
    cadenaPtr = cadena;
    close(1); // CIERRA STDOUT 
    open (cadenaPtr, O_CREAT | O_WRONLY, 0777); // CREA O ABRE EL ARCHIVO Y LO ASIGNA COMO SALIDA
}

/* CREA UN PROCESO HIJO Y UN PROCESO NIETO QUE SE ENCARGAN DE EJECUTAR LOS COMANDOS
   QUE FUERON SEPARADOS PREVIAMENTE, ESTO SUCEDE CUANDO EL COMANDO INGRESADO POSEE UNA 
   TUBERIA */
void EjecutarComandoTuberia(char **argumentos, char **argumentosTuberia){
    pid_t uno;
    uno = fork(); // CREA EL PRIMER HIJO
    if (uno < 0){
        printf("Error creando al hijo 1.\n");
        return;
    }

    if (uno == 0){
        
        int descriptorTuberia[2];
        pid_t dos;

        if(pipe(descriptorTuberia) < 0){ //INICIALIZA LA TUBERIA
        printf("Error. Tuberia no inicializada.\n");
        return;
        }

        dos = fork(); // CREA EL SEGUNDO HIJO (NIETO)

        if (dos < 0){
            printf("Error creando al hijo 2.\n");
            return;
        }

        if (dos == 0)
        {
            close(descriptorTuberia[0]); // CIERRA LA LECTURA DE LA TUBERIA 
            close(1); // CIERRA EL STDOUT
            dup(descriptorTuberia[1]); // ASIGNA LA ESCRITURA DE LA TUBERIA EN EL STDOUT 
            close(descriptorTuberia[1]); // CIERRA LA ESCRITURA DE LA TUBERIA

            if(execvp(argumentos[0], argumentos) < 0){ // EJECUTA EL COMANDO INGRESADO
                printf("Error ejecutando tuberia.\n");
                exit(0);
            }
            
        }else{

            wait(NULL); // ESPERA AL PROCESO HIJO 
            close(descriptorTuberia[1]); // CIERRA LA ESCRITURA DE LA TUBERIA
            close(0); // CIERRA EL STDIN
            dup(descriptorTuberia[0]); // ASIGNA LA LECTURA DE LA TUBERIA EN EL STDIN
            close(descriptorTuberia[0]); // CIERRA LA LECTURA DE LA TUBERIA

            if(execvp(argumentosTuberia[0], argumentosTuberia) < 0){ // EJECUTA EL COMANDO INGRESADO
                printf("Error ejecutando tuberia.\n");
                exit(0);
            }
        }
        
    }
    else{
        wait(NULL); //ESPERA AL PROCESO HIJO
    }
}

/* CREA UN PROCESO HIJO Y EJECUTA EL COMANDO SIMPLE INGRESADO */
int EjecutarComando(char **argumentos){
   
    pid_t id = fork(); // CREA EL PROCESO HIJO
    if(id == 0){
        if(execvp(argumentos[0],argumentos) < 0){ // EJECUTA EL COMANDO INGRESANDO
            printf("Error ejecutando.\n");
        } 
    }
    else if (id > 0){
        wait(NULL); // ESPERA AL PROCESO HIJO
    }else{
        printf("Error creando al hijo.\n");
        return -1;
    }
}

/* SE BUSCA LA EXISTENCIA DE UNA TUBERIA, DEPENDIENDO DEL RESULTADO SE PROCESAN LOS
   COMANDOS INGRESADOS Y RETORNA TIPO DE EJECUCION QUE DEBE EJECUTARSE */
int ProcesarString(char *lecturaConsola, char **argumentos, char **argumentosTuberia){
    char *stringTuberia[2];
    int tuberia = 0;
    tuberia = ProcesarTuberia(lecturaConsola, stringTuberia); // BUSCA SI HAY TUBERIA

    if (tuberia){
        ProcesarEspacios(stringTuberia[0], argumentos); // PROCESA EL PRIMER COMANDO DE UNA TUBERIA
        ProcesarEspacios(stringTuberia[1], argumentosTuberia); // PROCESA EL SEGUNDO COMANDO DE UNA TUBERIA
        return 1; // RETORNA EN CASO DE TUBERIA
    }
    else
    {
        ProcesarEspacios(lecturaConsola, argumentos); // PROCESA EL COMANDO SIN TUBERIA
        if(!strcmp(argumentos[0],"cd")){
            if (argumentos[1] == NULL)
            {
                chdir(homeDir);
            }else{
                chdir(argumentos[1]);
            }
            return 3; // RETORNA EN CASO DE CD
        }
        return 0; // RETORNA EN CASO SIN TUBERIA
    }
    return -1; // RETORNA EN CASO DE ERROR
}

void main(){

	IniciarShell();
 	// INICIALIZA LAS VARIABLES QUE SE UTILIZARAN
    char lecturaConsola[SIZE], // STRING PARA EL COMANDO DE ENTRADA
        *argumentos[COMANDOS], // ARRAY DE PUNTEROS A PARTES DEL COMANDO (ARGUMENTOS)
        *argumentosTuberia[COMANDOS]; // ARRAY DE PUNTEROS A PARTES DEL COMANDO (ARGUMENTOS)
    int stdOUT=dup(1), stdIN=dup(0); // RESPALDO DEL DESCRIPTOR DE ARCHIVO
    char *Simple[COMANDOS]; // ARRAY DE PUNTEROS DESPUES DE REDIRECCIONAMIENTO EN CASO SIN TUBERIA 
    char *ArgTuberia[COMANDOS]; // ARRAY DE PUNTEROS DESPUES DE REDIRECCIONAMIENTO EN CASO CON TUBERIA 

    while(1){
    	//REINICIA EL DESCRIPTOR DE ARCHIVO
    	close(1); // CIERRA STDOUT
    	dup(stdOUT); // ASIGNA EL RESPALDO EN EL STDOUT
    	close(0); // CIERRA STDIN
    	dup(stdIN); // ASIGNA EL RESPALDO EN EL STDIN

        Prompt(); // IMPRIME EL PROMPT

        if(RecibirString(lecturaConsola) == 0){ // OBTIENE CADENA INGRESADA
            if (!strcmp(lecturaConsola,"quit")){ // COMPRUEBA COMANDO DE SALIDA 
                printf("Cerrando shell.\n");
                exit(0); // CIERRA SHELL
            }
            
            // RECIBE TIPO DE EJECUCION 
            int ejecutar = ProcesarString(lecturaConsola, argumentos, argumentosTuberia);

            if (ejecutar == 0){ // EJECUCION SIN TUBERIA
            	int i=0;
            	while(i<COMANDOS){
            		if(argumentos[i]!=NULL){
            			// COMPRUEBA EXISTENCIA DE REDIRECCIONAMIENTO
            			if(strchr(argumentos[i],'>') || strchr(argumentos[i],'<') ){
            				if(strchr(argumentos[i],'>')){
            					// CONFIGURA DESCRIPTOR DE ARCHIVO EN CASO DE REDIRECCIONAMIENTO DE SALIDA
            					RedirigirSalida(argumentos[i+1]); 
            				}else{
            					// CONFIGURA DESCRIPTOR DE ARCHIVO EN CASO DE REDIRECCIONAMIENTO DE ENTRADA
            					RedirigirEntrada(argumentos[i+1]);
            				}
            				break;
            			}
            			Simple[i]=argumentos[i]; // ASIGNA ARGUMENTO A EJECUTAR 
            		}
            		else{
            			break;
            		}
            		i++;
            	}

                EjecutarComando(Simple); // EJECUTA COMANDO SIN TUBERIA 
                // "LIMPIA" ARGUMENTO SIMPLE
                while(i > 0){
                	Simple[i] = NULL;
                	i--;
                }

            }else if(ejecutar == 1){ // EJECUCION SIN TUBERIA
            	int i=0;
            	while(i<COMANDOS){
            		if(argumentos[i]!=NULL){
            			// COMPRUEBA EXISTENCIA DE REDIRECCIONAMIENTO
            			if(strchr(argumentos[i],'>') || strchr(argumentos[i],'<') ){
            				if(strchr(argumentos[i],'>')){
            					// CONFIGURA DESCRIPTOR DE ARCHIVO EN CASO DE REDIRECCIONAMIENTO DE SALIDA
            					RedirigirSalida(argumentos[i+1]);
            				}else{
            					// CONFIGURA DESCRIPTOR DE ARCHIVO EN CASO DE REDIRECCIONAMIENTO DE ENTRADA
            					RedirigirEntrada(argumentos[i+1]);
            				}
            				break;
            			}
            			Simple[i]=argumentos[i]; // ASIGNA ARGUMENTO A EJECUTAR 
            		}
            		else{
            			break;
            		}
            		i++;
            	}
            	// REVISA ARGUMENTOSTUBERIA 
            	i=0;
            	while(i<COMANDOS){
            		if(argumentosTuberia[i]!=NULL){
            			// COMPRUEBA EXISTENCIA DE REDIRECCIONAMIENTO
            			if(strchr(argumentosTuberia[i],'>') || strchr(argumentosTuberia[i],'<') ){
            				if(strchr(argumentosTuberia[i],'>')){
            					// CONFIGURA DESCRIPTOR DE ARCHIVO EN CASO DE REDIRECCIONAMIENTO DE SALIDA
            					RedirigirSalida(argumentosTuberia[i+1]);
            				}else{
            					// CONFIGURA DESCRIPTOR DE ARCHIVO EN CASO DE REDIRECCIONAMIENTO DE ENTRADA
            					RedirigirEntrada(argumentosTuberia[i+1]);
            				}
            				break;
            			}
            			ArgTuberia[i]=argumentosTuberia[i]; // ASIGNA ARGUMENTO TUBERIA A EJECUTAR 
            		}
            		else{
            			break;
            		}
            		i++;
            	}
            	
            	EjecutarComandoTuberia(Simple,ArgTuberia); // EJECUTA COMANDO CON TUBERIA 
            	// "LIMPIA" ARGUMENTO SIMPLE Y ARGTUBERIA
            	while(i > 0){
                	Simple[i] = NULL;
                	ArgTuberia[i] = NULL;
                	i--;
                }
            }
        }
    }
}