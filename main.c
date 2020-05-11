/* Ejemplo de manejo de un teclado multeplexado para subir y bajar un objeto con el ESP32
 
   Conexiones:
   * columnas 1 a 4 GPIO32, GPIO33, GPIO25, GPIO26
   * renglones 1 a 4 GPIO27, GPIO2, GPIO4, GPIO17
   * sensores por piso GPIO19, GPIO21, GPIO5
   * motores GPIO12 y GPIO13
   * Los renglones se configuran como salidas y las columnas como
   * entradas con PULL-DOWN 
   
   El presente ejemplo se publica sin niguna garantia expresa o 
   implicita de funcionamiento.
*/
//Modificaciòn de Dulce
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define GPIO_COLUMNA_1    32
#define GPIO_COLUMNA_2    33
#define GPIO_COLUMNA_3    25
#define GPIO_COLUMNA_4    26
#define GPIO_RENGLON_1    27
#define GPIO_RENGLON_2    2
#define GPIO_RENGLON_3    4
#define GPIO_RENGLON_4    17
#define MOTOR1 GPIO_NUM_12
#define MOTOR2 GPIO_NUM_13
#define S0 GPIO_NUM_19
#define S1 GPIO_NUM_5
#define S2 GPIO_NUM_21

uint32_t dir, signo;
#define GPIO_RENGLONES_SEL  ((1ULL<<GPIO_RENGLON_1) | (1ULL<<GPIO_RENGLON_2) | (1ULL<<GPIO_RENGLON_3) | (1ULL<<GPIO_RENGLON_4))
#define GPIO_COLUMNAS_SEL  ((1ULL<<GPIO_COLUMNA_1) | (1ULL<<GPIO_COLUMNA_2) | (1ULL<<GPIO_COLUMNA_3) | (1ULL<<GPIO_COLUMNA_4))

void Motor(int dir);

void Motor(int dir)
{
	printf("Motor %d\n",dir);
	switch(dir)
	{
		case 0: 
		gpio_set_level(MOTOR1, 0);
		gpio_set_level(MOTOR2, 1); 
		break; 
		case 1: 
		gpio_set_level(MOTOR1, 1);
		gpio_set_level(MOTOR2, 0);
		break; 
		case 2: 
		gpio_set_level(MOTOR1, 0);
		gpio_set_level(MOTOR2, 0);
		break; 
	}
	return; 
}

/*static void ArribaMotor()
{
	gpio_set_level(MOTOR1, 1);
        gpio_set_level(MOTOR2, 0);
	return; 
}
static void PararMotor()
{
	gpio_set_level(MOTOR1, 0);
        gpio_set_level(MOTOR2, 0);
	return; 
}
static void AbajoMotor()
{
	gpio_set_level(MOTOR1, 0);
        gpio_set_level(MOTOR2, 1); 
	return; 
} */
int Direccion(int signo)
{
	if(signo<=0)
	{
		dir=0; 
	}
	else if(signo>0)
	{
		dir=1; 
	}
	return dir;
}

//Define las colas para la comunicación de eventos del teclado 
//static QueueHandle_t xFILATeclado;
static xQueueHandle xFILATeclado = NULL;

// define la tarea del teclado y la del ADC
static void TaskTeclado( void *pvParameters );

//#define ESP_INTR_FLAG_DEFAULT 0


void app_main(void)
{
    uint32_t tecla;
    uint32_t pisoAnterior,piso;
    pisoAnterior=0; 
    gpio_config_t io_conf;

gpio_pad_select_gpio(MOTOR1); 
gpio_pad_select_gpio(MOTOR2);
gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT);
gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);
gpio_set_direction(S0, GPIO_MODE_INPUT);
gpio_set_pull_mode(S0, GPIO_PULLUP_ONLY); 
gpio_set_direction(S1, GPIO_MODE_INPUT);
gpio_set_pull_mode(S1, GPIO_PULLUP_ONLY); 
gpio_set_direction(S2, GPIO_MODE_INPUT);
gpio_set_pull_mode(S2, GPIO_PULLUP_ONLY); 
       
    //Configura los renglones como salida
    
    //deshabilita las interrupciones
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //Configura como salida
    io_conf.mode = GPIO_MODE_OUTPUT;
    //Mascara de bits de las terminales que se desea configurar
    io_conf.pin_bit_mask = GPIO_RENGLONES_SEL;
    //deshabilita el modo pull-down
    io_conf.pull_down_en = 0;
    //habilita el pull-up 
    io_conf.pull_up_en = 0;
    //configura las GPIO con los valores indicados
    gpio_config(&io_conf);
    
    
    //Configura las columnas como entrada con pull-down
    
    //habilita las interrupciones con flanco de bajada
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //Configura como salida
    io_conf.mode = GPIO_MODE_INPUT;
    //Mascara de bits de las terminales que se desea configurar
    io_conf.pin_bit_mask = GPIO_COLUMNAS_SEL;
    //habilita el modo pull-down
    io_conf.pull_down_en = 1;
    //deshabilita el pull-up 
    io_conf.pull_up_en = 0;
    //configura las GPIO con los valores indicados
    gpio_config(&io_conf);
    
    //crea el FILA del teclado
    xFILATeclado = xQueueCreate(10, sizeof(int));
    
    //start gpio task
    xTaskCreate(TaskTeclado, "TareaTeclado", 2048, NULL, 10, NULL);
	printf("entrando al ciclo principal\n");

    while(1) 
{
        //Si se presiona una tecla en los siguientes dos Tics,
        //la muestra 

        if(xQueueReceive(xFILATeclado, &tecla, 1000)) 
	{
            printf("Se presiono: %d\n", tecla);
        }
	switch(tecla)
	{
		case 0: 
		piso=0; 
		signo=piso-pisoAnterior; //Se calcula si es negativa o positiva la dirección respecto al piso anterior.
		Direccion(signo); //Se da valor para indicar dirección
		Motor(dir); //Se manda al motor si va hacia arriba o hacia abajo.
		while(1){
			int estado=gpio_get_level(S0);
			//printf("estado %d\n",estado);
			if(estado==0) break;
			vTaskDelay(1);
		}
		Motor(2); //Se para el motor.
		break;

		case 1:
		piso=1;
		signo=piso-pisoAnterior; 	
		Direccion(signo); 
		Motor(dir);
		while(1){
			int estado=gpio_get_level(S1);
			//printf("estado %d\n",estado);
			if(estado==0) break;
			vTaskDelay(1);
		}
		Motor(2); 
		break;
		
		case 2:
		piso=2;
		signo=piso-pisoAnterior;
		Direccion(signo); 
		Motor(dir);
		while(1){
			int estado=gpio_get_level(S2);
			//printf("estado %d\n",estado);
			if(estado==0) break;
			vTaskDelay(1);
		}
		Motor(2);
		break;

		default:
		break;
	}
    }
}


//arreglo con los numeros de GPIO de las diferentes columnas y renglones del
//Teclado multiplexado
int columnas[]={GPIO_COLUMNA_1,GPIO_COLUMNA_2,GPIO_COLUMNA_3,GPIO_COLUMNA_4};
int renglones[]={GPIO_RENGLON_1,GPIO_RENGLON_2,GPIO_RENGLON_3,GPIO_RENGLON_4};

//Tarea de exploración del teclado

static void TaskTeclado(void *pvParameters)  
{
  int renglon,columna,teclaPresionada;
  int codigo,codigoAnterior;
  //(void) pvParameters;
  
  codigoAnterior=-1;
  
  for(renglon=0;renglon<3;renglon++)
    gpio_set_level(renglones[renglon],0);
  
  for (;;) // una tarea nunca debe retornar o salir
  {
    teclaPresionada=0;//supone que no hay una tecla presionada
    codigo=0;
    //exploracion del teclado multiplexado
    for(renglon=0;renglon<3;renglon++)
   {
      gpio_set_level(renglones[renglon],1);
      for(columna=0;columna<3;columna++)
     {
          //printf("r:%d c: %d in:%d\n",renglon,columna,gpio_get_level(columnas[columna]));
          if(gpio_get_level(columnas[columna])==1) 
	  {
             teclaPresionada=1;
             codigo=(columna)|(renglon<<2);
          }
      }
      gpio_set_level(renglones[renglon],0);
    }
    
    //eliminacion de rebotes
    if(teclaPresionada==1)
   {
      if (codigo==codigoAnterior)
      {
        //meter a la cola el codigo de la tecla
        xQueueSendToBack(xFILATeclado,&codigo,portMAX_DELAY);
        vTaskDelay( 150 / portTICK_RATE_MS ); // espera durante 150mst
      }
      codigoAnterior=codigo;
      vTaskDelay( 50 / portTICK_RATE_MS ); // espera durante 50 ms
   }
    else
    {
      codigoAnterior=-1;
      vTaskDelay( 20 / portTICK_RATE_MS ); // espera durante 20ms 
    }
  }
}
