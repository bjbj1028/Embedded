#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <mysql/mysql.h>
#include <time.h>
#include <math.h>
#include <wiringPiSPI.h>
#include <pthread.h>
#define CS_MCP3208 8
#define SPI_CHANNEL 0
#define SPI_SPEED 1000000
#define VCC 4.8

#define MAXTIMINGS 85
#define RETRY 5
#define FAN 22
#define RGBLEDPOWER 24
#define LIGHTSEN_ OUT 2

#define DBHOST "localhost"
#define DBUSER "root"
#define DBPASS "root"
#define DBNAME "demofarmdb"

#define MAX 10

MYSQL *connector;
MYSQL_RES *result;
MYSQL_ROW row;

int delaycounter = 0;
int Tbuffer[MAX];
int Lbuffer[MAX];
int fill_ptr = 0;
int Luse_ptr = 0;
int Tuse_ptr = 0;
int Lcount = 0;
int Tcount = 0;
int loops = 1000;
int done = 0;
int ret_temp;
static int DHTPIN = 11;
static int dht22_dat[5] = {0,0,0,0,0};

pthread_cond_t empty, fill;
pthread_mutex_t mutex;

void put(int Lvalue, int Tvalue)
{
				Lbuffer[fill_ptr] = Lvalue;
				Tbuffer[fill_ptr] = Tvalue;
				fill_ptr = (fill_ptr+1)%MAX;
				Lcount++;
				Tcount++;
}
int Lget()
{
				int tmp = Lbuffer[Luse_ptr];
				Luse_ptr = (Luse_ptr +1) % MAX;
				Lcount--;
				return tmp;
}
int Tget()
{
				int tmp = Tbuffer[Tuse_ptr];
				Tuse_ptr = (Tuse_ptr + 1) % MAX;
				Tcount--;
				return tmp;
}



static uint8_t sizecvt(const int read)
{
				if(read > 255 || read <0)
				{
								printf("Invalid data from wiringPi library\n");
								exit(EXIT_FAILURE);
				}
				return (uint8_t)read;
}
int read_dht22_dat()
{
				uint8_t laststate = HIGH;
				uint8_t counter = 0 ;
				uint8_t j = 0 , i;

				dht22_dat[0] = dht22_dat[1] = dht22_dat[2] = dht22_dat[3] = dht22_dat[4] = 0;
				pinMode(DHTPIN, OUTPUT);
				digitalWrite(DHTPIN, HIGH);
				delay(10);
				digitalWrite(DHTPIN, LOW);
				delay(18);

				digitalWrite(DHTPIN, HIGH);
				delayMicroseconds(40);
				pinMode(DHTPIN, INPUT);

				for(i = 0 ; i < MAXTIMINGS ; i++)
				{
								counter = 0 ;
								while(sizecvt(digitalRead(DHTPIN))==laststate){
												counter++;
												delayMicroseconds(1);
												if (counter == 255){
																break;
												}
								}
								laststate = sizecvt(digitalRead(DHTPIN));

								if(counter == 255) break;

								if((i>=4)&&(i%2==0)){
												dht22_dat[j/8] <<= 1;
												if(counter > 50)
																dht22_dat[j/8] |= 1;
												j++;
								}
				}
				if((j>=40)&&(dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] +dht22_dat[2]+ dht22_dat[3])&0xFF)))
				{
								float t;
								t = (float)(dht22_dat[2] & 0x7F)*256 + (float)dht22_dat[3];
								t /= 10.0;
								if ((dht22_dat[2] & 0x80) != 0) t *= -1;

				ret_temp = (int)t;
				return ret_temp;
				}
				else
				{
								printf("Data not good, skip\n");
								return 0;
				}
}

int read_mcp3208_abc(unsigned char abcChannel)
{
				unsigned char buff[3];
				int abcValue = 0;

				buff[0] = 0x06 | ((abcChannel & 0x07) >> 2);
				buff[1] = ((abcChannel & 0x07) << 6);
				buff[2] = 0x00;

				digitalWrite(CS_MCP3208, 0);
				wiringPiSPIDataRW(SPI_CHANNEL, buff, 3);

				buff[1] = 0x0f & buff[1];
				abcValue = (buff[1] << 8) | buff[2];
				digitalWrite(CS_MCP3208, 1);
				return abcValue;
}

void sig_handler(int signo)
{
				printf("process stop\n");
				digitalWrite (FAN, 0);
				digitalWrite (RGBLEDPOWER, 0);
				exit(0);
}
void *LED(void *arg)
{
				int i = 0;
				signal(SIGINT, (void *)sig_handler);
				if(wiringPiSetup () == -1)
				{
								fprintf(stdout, "Unable to start wiringPi : %s\n", strerror(errno));
				}
				for(i = 0 ; i<loops ; i++)
				{
								pthread_mutex_lock(&mutex);
								while(Lcount == 0)
												pthread_cond_wait(&fill, &mutex);
								int tmp = Lget();
								pthread_cond_signal(&empty);
								pthread_mutex_unlock(&mutex);
								printf("LED Start\n");
				pinMode(RGBLEDPOWER, OUTPUT);
				printf("Lget : %d\n", tmp);
				if(tmp > 1500)
				{
								printf("LED ON\n");
				digitalWrite(RGBLEDPOWER, 1);
				delay(500);
				digitalWrite(RGBLEDPOWER,0);
				}
				printf("LED stop\n");
				}
}
void *Fan(void *arg)
{
				int i = 0;
				signal(SIGINT, (void *)sig_handler);
				if(wiringPiSetup() == -1)
				{
								fprintf(stdout, "Unable to start wiringPi : %s\n", strerror(errno));
							
				}
				for(i = 0 ; i < loops ; i++ )
				{
								pthread_mutex_lock(&mutex);
								while(Tcount == 0)
												pthread_cond_wait(&fill, &mutex);
								int tmp = Tget();
								pthread_cond_signal(&empty);
								pthread_mutex_unlock(&mutex);
								printf("FAN START \n");
								printf("%d\n",tmp);
				pinMode (FAN, OUTPUT);
				
				
								if(tmp > 35)
								{
												printf("FAN ON\n");
								digitalWrite(FAN,1);
								delay(5000);
								delaycounter += 5000;
								digitalWrite(FAN,0);
								}
				
				printf("FAN STOP\n");
				}
				
}
void *Monitor(void *arg)
{
				int i = 0 ;
				unsigned char abcChannel_light = 0;
				int abcValue_light = 0;
				float vout_light;
				float vout_oftemp;
				float percentrh = 0;
				float supsiondo = 0;
				int received_temp;
				for(i = 0 ; i < loops ; i++)
				{
				if(wiringPiSetup() == -1)
								exit(EXIT_FAILURE);
				if (setuid(getuid())<0)
				{
								perror("Dropping privileges failed\n");
								exit(EXIT_FAILURE);
				}
				
				if(wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1)
				{
								fprintf(stdout, "wiringPiSPISetup Failed : %s\n", strerror(errno));
								
				}
				pinMode(CS_MCP3208, OUTPUT);
				while(read_dht22_dat() == 0)
				{
								delay(500);
								delaycounter += 500;
				}
				received_temp = ret_temp;
			printf("thread1 start\n");	
								abcValue_light = read_mcp3208_abc(abcChannel_light);
								printf("light sensor = %u\n", abcValue_light);
				printf("Temperature = %d\n", received_temp);
				
				pthread_mutex_lock(&mutex);
				while(Lcount == MAX || Tcount == MAX)
								pthread_cond_wait(&empty, &mutex);
				put(abcValue_light,received_temp);
				printf("thread1 put : %d, %d\n",abcValue_light, received_temp);
				pthread_cond_signal(&fill);
				pthread_mutex_unlock(&mutex);
				printf("thread1 stop\n");

				}
}
				

				

void *INSERTDB(void *arg)
{
	  int i = 0 ;
		char query[1024];
		connector = mysql_init(NULL);
		if(!mysql_real_connect(connector, DBHOST, DBUSER, DBPASS, DBNAME, 3306, NULL, 0))
		{
						fprintf(stderr, "%s\n", mysql_error(connector));
		}
		printf("MYSLQ opened\n");
		for(i = 0 ; i < loops ; i ++)
		{
						pthread_mutex_lock(&mutex);
						while(Lcount == 0)
										pthread_cond_wait(&fill, &mutex);
						int tmp = Lbuffer[Luse_ptr];
						int tmp2 = Tbuffer[Tuse_ptr];
						
						pthread_mutex_unlock(&mutex);
						
						
						printf("Store Data Light : %d, Temperature : %d\n", tmp, tmp2);
						sprintf(query, "insert into thl values(now(), %d, %d)",tmp2, tmp);
						if(mysql_query(connector, query))
						{
										fprintf(stderr, "%s\n", mysql_error(connector));
										printf("Write DB error\n");
						}
						pthread_cond_signal(&empty);
						delay(10000);
						
		}
}


int main(int argc, char *argv[])
{
				pthread_t p1, p2, p3, p4;
			  pthread_create(&p1,NULL,Monitor,NULL);
				pthread_create(&p2,NULL,Fan,NULL);
				pthread_create(&p3,NULL,LED, NULL);
				pthread_create(&p4, NULL, INSERTDB, NULL);
				pthread_join(p1,NULL);
				pthread_join(p2,NULL);
				pthread_join(p3,NULL);
				pthread_join(p4,NULL);
				/*int received_temp;
				if (wiringPiSetup() == -1)
								exit(EXIT_FAILURE);
				while(read_dht22_dat()==0)
				{
								delay(500);
				}
				received_temp = ret_temp;
				printf("Temperature = %d\n",received_temp);*/
				mysql_close(connector);
				return 0;
}
